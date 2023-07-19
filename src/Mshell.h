/**
 * MDH@27FEB2020: every evaluation of an M command takes place 'inside' an M shell 
 *                which basically creates the top level M environment with all it's predefined constants and functions
 */
#include <limits.h>

#include "Menvironment.h"

// Decimal support
long long getDP();

Mvalue* getdc(Mvalue* value);

Mvalue* getdp(Mvalue* value);
Mvalue* setdp(Mvalue* value);

// end Decimal stuff

// Mcommand stuff
// MDH@28OCT2019: because now often we need both the first and last token in a command it's probably best to combine them in a single command
typedef struct{
	unsigned long long sourceCommandIndex; // MDH@18JUN2020: storing what this command is a duplicate of
	Mtoken *_firstToken,*_lastToken;
	/////////////////bool identifierContinuationIsDirty; // convenient to keep it with the command itself
}Mcommand;

Mcommand* owned_command(Mcommand* _command,Mallocationowner owner_command);
Mcommand* disowned_command(Mcommand* _command,Mallocationowner owner_command);
void free_command(Mcommand* _command/*,Mallocationowner owner*/);
#define FREE_COMMAND(_command,owner_command) free_command(disowned_command(_command,owner_command))

void setTokenType(Mtoken* token,TokenType tokenType/*,bool endOfInput*/);
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType/*,bool endOfInput*/); // prototype
Mcommand* _getNewCommand(bool withFirstToken);

// MDH@20FEB2020: the function definition so we can plug in our own inputInfo and inputError functions
typedef void InputResponseFunction(char const * const fmt,...);
typedef size_t OutputTokenFunction(Mtoken const * const token);
typedef void ReoutputTokenFunction(Mtoken const * const token);
typedef void UpdateLastTokenAutocompletionTextFunction(bool onlyWhenItDoesNotEndTheSuggestedText);
typedef void OutputCommandInfoFunction(Mcommand const * const command);
typedef bool InputCharReadFunction(char* _c);
/*
void setInputInfoFunction(InputResponseFunction* _inputResponseFunction);
void setInputErrorFunction(InputResponseFunction* _inputResponseFunction);
// MDH@28FEB2020: for now being able to plug in a reoutput token function solves the problem of having to reoutput a token when typed by the user in a interactive session
void setReoutputTokenFunction(ReoutputTokenFunction* _reoutputTokenFunction);
void setUpdateLastTokenAutocompletionTextFunction(UpdateLastTokenAutocompletionTextFunction* _updateLastTokenAutocompletionTextFunction);
void setOutputCommandInfoFunction(OutputCommandInfoFunction* _outputCommandInfoFunction);
*/
bool isOneCharacterTokenType(uint8_t tokenType);

// the list of token type ids in the corresponding order!!!
/* MDH@04MAR2020: no color use in the shell
const char* getTokenColor(enum TOKENTYPE_ENUM tokenType);
void outputTokenTypeColor(TokenType tokenType);
void outputTokenColor(Mtoken* _token);
size_t outputToken(Mtoken* _token);
*/
void outputLastTokenChar(Mtoken* token);
// does not need to be exposed as it's only used internally: Mtoken* freeToken(Mtoken* _token);
Mtoken* removedLastCommandToken(Mcommand* command,Mallocationowner owner_command); // needs to be here, as it is used by isAValidCommand() to cut off comments and errors
// for appending input characters to the end of a given command (as used by Mevalfunction) and commandCharacterAccepted() in M.c

int8_t containsVariable(Menvironment const * const _environment,char /*const*/ * const name, int8_t report); // MDH@12MAR2020: used in changeFunctionTokenToAVariable() (moved from Menvironment.h/c)

void changeFunctionTokenToAVariable(Mcommand* command,bool endOfInput);
bool existsInCommand(Mcommand* command,char* identifierName,uint64_t identifierEnvironmentId);
bool existsAsLocalVariable(char* identifierName,uint64_t identifierEnvironmentId); // MDH@25FEB2021: expose to the outside (in particular called from M.c)
Mtoken* commandCharacterAppended(Mcommand* command/*,Mallocationowner owner_command*/,char inputChar,char *inputCharacterType,bool endOfInput);

// MDH@19OCT2020: some functions that we can use to determine what a character would do to the current token
bool characterContinuesToken(Mtoken const * const token,char inputChar,char inputCharacterType);

// MDH@18JUL2023: owner_command no longer required by isAValidCommandIndicator!!!
int8_t isAValidCommandIndicator(Mcommand const * const command/*,Mallocationowner owner_command*/,bool report); // returns negative values for invalid commands, 0 for invalid input, positive value for valid commands

Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount);

// MDH@20FEB2020 only called inside Mshell.c so removed from this header file: Mvalue* getCommandValue(Mcommand* command,char commandType);

typedef struct FunctionBodyRequest{
	Mchars* _functionName; // MDH@02MAR2020: allocated on the heap so starts with _ now (to indicate that it should be freed together with its wrapper)
	struct FunctionBodyRequest *_next;
}FunctionBodyRequest;
FunctionBodyRequest* getFirstFunctionBodyRequest();
typedef struct FunctionBodyInput{
	////////char* functionName;
	Muserfunction* _function;
	struct FunctionBodyInput* _prev;
	struct FunctionBodyRequest* _request;
}FunctionBodyInput;
FunctionBodyInput* getCurrentFunctionBodyInput(); // exposing the current function body input
bool startFunctionBodyInput();
bool endFunctionBodyInput();
bool createFunctionBodyInput(FunctionBodyRequest const * const _functionBodyRequest);

bool settingApplied(char settingCharacter);

// MDH@28OCT2020: moved over from Menvironment.c/h
// MDH@29OCT2020: added _localMapValue to the anonymous function call
Mvalue* Manonymousfunction(Mvalue* _parameterMapValue,Mvalue* _localMapValue,Mvalue* _bodyValue);
Mvalue* Mdefinefunction(Mvalue* _nameValue,Mvalue* _parameterMapValue,Mvalue* _bodyValue);

// MDH@28OCT2020: the famous array functions reduce, map, filter and foreach
Mvalue* Mlreduce(Mvalue* _listValue,Mvalue* _functionValue,Mvalue* _initialAccumulatedValue);
Mvalue* Mlmap(Mvalue* _listValue,Mvalue* _functionValue);
Mvalue* Mlfilter(Mvalue* _listValue,Mvalue* _functionValue);
Mvalue* Mlforeach(Mvalue* _listValue,Mvalue* _functionValue);
Mvalue* Mlgroup(Mvalue* _listValue,Mvalue* _functionValue);

Mvalue* Mcorr(Mvalue* _sequence1Value,Mvalue* _sequence2Value);
Mvalue* Mstats(Mvalue* _sequenceValue);

void outputCommandInfo(Mcommand const * const _command);

// and finally obtaining a root environment
// in general initializing the shell with the callbacks and the M setting characters
// MDH@05DEC2020: added moduleDebugging flags
bool shellInitialized(
				char const * const settingCharacters,
				char const * const locale,
				unsigned long long moduleDebugging,
				InputCharReadFunction _inputCharReadFunction,
				InputResponseFunction _inputInfoFunction,
				InputResponseFunction _inputErrorFunction,
				OutputTokenFunction _outputTokenFunction,
				ReoutputTokenFunction _reoutputTokenFunction,
				UpdateLastTokenAutocompletionTextFunction* _updateLastTokenAutocompletionTextFunction,
				OutputCommandInfoFunction _outputCommandInfoFunction
			);


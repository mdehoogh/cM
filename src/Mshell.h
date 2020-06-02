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

mpd_context_t* get_default_mpd_context();
// end Decimal stuff

// Mcommand stuff
// MDH@28OCT2019: because now often we need both the first and last token in a command it's probably best to combine them in a single command
typedef struct{
	Mtoken *_firstToken,*_lastToken;
	/////////////////bool identifierContinuationIsDirty; // convenient to keep it with the command itself
}Mcommand;
void free_command(Mcommand* _command,Mallocationowner owner);
void setTokenType(Mtoken* token,TokenType tokenType/*,bool endOfInput*/);
Mtoken* _getNewCommandToken(Mtoken* lastCommandToken,TokenType tokenType/*,bool endOfInput*/); // prototype
Mcommand* _getNewCommand(bool withFirstToken);

// MDH@20FEB2020: the function definition so we can plug in our own inputInfo and inputError functions
typedef void InputResponseFunction(char const * const fmt,...);
typedef size_t OutputTokenFunction(Mtoken* token);
typedef void ReoutputTokenFunction(Mtoken* token);
typedef void UpdateLastTokenAutocompletionTextFunction();
typedef void OutputCommandInfoFunction(Mcommand* command);
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
Mtoken* commandCharacterAppended(Mcommand* command,char inputChar,char *inputCharacterType,bool endOfInput);

int8_t isAValidCommandIndicator(Mcommand* command,Mallocationowner owner_command,bool report); // returns negative values for invalid commands, 0 for invalid input, positive value for valid commands

Mvalue* getValueOfExpression(const char* info,char resulttype,TokenType endTokenTypes[],uint8_t endTokenTypeCount);

// MDH@20FEB2020 only called inside Mshell.c so removed from this header file: Mvalue* getCommandValue(Mcommand* command,char commandType);

typedef struct FunctionBodyRequest{
	char* _functionName; // MDH@02MAR2020: allocated on the heap so starts with _ now (to indicate that it should be freed together with its wrapper)
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

// and finally obtaining a root environment
// in general initializing the shell with the callbacks and the M setting characters
bool shellInitialized(
				char const * const settingCharacters,
				InputCharReadFunction _inputCharReadFunction,
				InputResponseFunction _inputInfoFunction,
				InputResponseFunction _inputErrorFunction,
				OutputTokenFunction _outputTokenFunction,
				ReoutputTokenFunction _reoutputTokenFunction,
				UpdateLastTokenAutocompletionTextFunction* _updateLastTokenAutocompletionTextFunction,
				OutputCommandInfoFunction _outputCommandInfoFunction
			);


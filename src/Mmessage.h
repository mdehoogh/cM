/**
 * MDH@27FEB2020: methods to output M messages to standard output as used by most modules
 *                moved over from Msession.c and Mexecution.c so we can move Msession.c up (and called from within M.c only)
 */
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "Moutput.h"

/* possibly move to a separate module in the future
// support for multiple message streams stored in a double-linked message stream list (_messageStreamStack)
bool pushMessageStream(FILE* stream,char const * const source,char const * const messageType);
bool popMessageStream(char const * const source,char const * const messageType);
bool popAllMessageStreams(char const * const source);

bool messageStreamsInitialized(char const * const source);
*/
typedef struct Message{
	size_t index;
	char* id; // the id of the message identifying the group it belongs to
	char* msg;
}Message;

typedef struct Messages{
	size_t count; // the number of message nodes in the message array
	char** types; // the types of the messages (the fastest way to return the types as well)
	Message** messages; // the array of messages
}Messages;

typedef struct MessageCount{
	size_t count;
	char* messageType;
}MessageCount;

typedef struct MessageCounts{
	size_t count;
	MessageCount* messagecounts;
}MessageCounts;

////////Messages* _getMessages(); // MDH@20AUG2024: for retrieving all messages (see also Mmessages() user function)
Messages* _getFilteredMessages(MessageCounts * const messageCounts);
// replacing: Messages* _getMessagesOfType(char const * const messageType); // exposes messages of a certain type
void freeMessages(Messages* messages);
long long removeMessages(MessageCounts const * const messageCounts);
// MDH@28AUG2024: for retrieving and freeing message counts!!
MessageCounts* _getMessageCounts();
void freeMessageCounts(MessageCounts* messageCounts);

bool addMessageOfType(char const * const messageText,char const * const messageType);

char* setMessageId(char const * const messageId); // echoes the messageId

////////size_t logMessage(char const * const messageType, char const * const messagefmt,...);

// MDH@07AUG2024: for logging any text to either output and/or collect (that may contain any message types)
size_t q2collect(char const * const fmt,...);
size_t q2outputandcollect(char const * const fmt,...);
size_t q2newline(bool echoToOutput);
// MDH@20SEP2024: most convenient to use outputMessage() to output a single message of a specific type
//                that outputError/Bug/Warning can delegate to
size_t outputMessage(char const * const messageType,char const * const fmt,...);
size_t collectMessage(char const * const messageType,char const * const fmt,...);

bool outputCollectorInitialized();

// might copy these to Moutput.c/h and create queued versions
// MDH@19AUG2024: changed to queued (collected) versions because we want these messages collected as well!!!
size_t outputInfo(char const * const info); // replacing outputLine in all modules

size_t outputWarning(char const * const warning);

size_t outputError(char const * const error);
size_t outputErrorAndText(char const * const error,char const * const text);
// MDH@05NOV2019: some special error reporting (typically bugs and out of memory problems)
size_t outputMemoryError(char const * const memoryerror);

size_t outputBug(char const * const bug);

int kbhit();
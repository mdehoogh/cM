#include <string.h>

#include <unistd.h>
#include <termios.h>

#include "Mmessage.h"

/// the ID of this module
static int32_t const MODULE_ID=(2<<4);

// the texts to be used in certain message types
extern const char* const M_INFO_PREFIX;
extern const char* const M_ERROR_PREFIX;
extern const char* const M_WARNING_PREFIX;
extern const char* const M_BUG_PREFIX;

typedef struct MessageStream{
	char* source;
	char* messageType; // the type of the messages it is allowed to receive
	FILE* stream;
	struct MessageStream* prev;
	struct MessageStream* next;
}MessageStream;

static MessageStream* _messageStreamStack=NULL; // points to the start of the output message stack

/**
 * @brief returns the message stream with source \p source and message type \p messageType
 * @details if \p messageType is defined, if there's a stream with that message type it is preferred over the 'general' message type message stream
 * @param source 
 * @param messageType 
 * @return MessageStream* the message stream with source \p source and message type \p messageType
 */
MessageStream* getMessageStream(char const * const source,char const * const messageType){
	MessageStream* messageStream=(NULL==source?NULL:_messageStreamStack);
	while(messageStream!=NULL){
		if(strcmp(source,messageStream->source)==0){ // the source matches
			// the message types needs to match as well
			if(messageType!=NULL){
				if(strcmp(messageType,messageStream->messageType))break;
			}else{
				if(NULL==messageStream->messageType)break;
			}
		}
		messageStream=messageStream->next;
	}
	return messageStream;
}
/**
 * @brief pushes a message stream with stream \p stream from source \p source and message type \p messageType on the message stream stack
 * @details does NOT replace when a message stream with source \p source and message type \p messageType already exists
 * @param stream 
 * @param source 
 * @param messageType 
 * @return true on success
 * @return false on failure
 */
bool pushMessageStream(FILE* stream,char const * const source,char const * const messageType){
	MessageStream* messageStream=NULL;
	if(source!=NULL&&stream!=NULL){
		MessageStream* messageStream=getMessageStream(source,messageType);
		if(NULL==messageStream){ // haven't got it yet
			messageStream=calloc(1,sizeof(MessageStream));
			if(messageStream!=NULL){
				messageStream->stream=stream;
				messageStream->source=strdup(source);
				if(messageType!=NULL)messageStream->messageType=strdup(messageType);
				if(messageStream->source!=NULL&&(messageType==NULL||messageStream->messageType!=NULL)){ // created successfully
					if(_messageStreamStack!=NULL){ // prepend to _messageStreamStack
						messageStream->next=_messageStreamStack;
						_messageStreamStack->prev=messageStream;
					}
					_messageStreamStack=messageStream;
				}else{
					free(messageStream);messageStream=NULL;
					output("%sFailed to register message stream '%s'.\n",M_ERROR_PREFIX,source);
				}
			}
		}
	}
	return(messageStream!=NULL);
}
static bool removeMessageStream(MessageStream* messageStream){
	if(messageStream!=NULL){
		MessageStream* prevMessageStream=messageStream->prev;
		MessageStream* nextMessageStream=messageStream->next;
		if(messageStream->source)free(messageStream->source);
		if(messageStream->messageType)free(messageStream->messageType);
		fflush(messageStream->stream); // just in case
		free(messageStream);
		if(prevMessageStream!=NULL)prevMessageStream->next=nextMessageStream;else _messageStreamStack=nextMessageStream;
		if(nextMessageStream!=NULL)nextMessageStream->prev=prevMessageStream;
	}
	return false;
}
/**
 * @brief 
 * 
 * @param source 
 * @return true 
 * @return false 
 */
bool popMessageStream(char const * const source,char const * const messageType){
	MessageStream* messageStream=getMessageStream(source,messageType);
	return(messageStream!=NULL&&removeMessageStream(messageStream));
}
/**
 * @brief pops all message streams associated with source \p source from the message stream stack
 * 
 * @param source the name of the message streams to remove
 * @return true on success
 * @return false on failure
 */
bool popAllMessageStreams(char const * const source){
	if(source!=NULL){
		MessageStream* messageStream=_messageStreamStack;
		while(messageStream!=NULL){
			MessageStream* nextMessageStream=messageStream->next;
			if(strcmp(messageStream->source,source)==0&&!removeMessageStream(messageStream)){
				output("%sFailed to remove a message stream of source '%s'.\n",M_ERROR_PREFIX,source);
				break;
			}
			messageStream=nextMessageStream;
		}
		if(NULL==messageStream)return true;
	}
	return false;
}
// MDH@25MAY2024: in order to be able to catch messages, you should use outputMessage
//                wondering where to send the output message to
//                this is a 'generic' function in that it outputs to all the registered output streams
//                but perhaps there should be an output stream for each of the message types
size_t logToMessageStream(char const * const messageType,char const * const messagefmt,...){
	size_t result=0;
	MessageStream* messageStream=_messageStreamStack;
	while(messageStream!=NULL){
	  va_list args;
  	va_start(args,messagefmt);
		size_t written=0;
		if(messageType==NULL||strcmp(messageType,messageStream->messageType)==0){
			// only write the message type as prefix when not expected by the message stream
			if(messageType!=NULL&&messageStream->messageType==NULL)written=vfprintf(messageStream,"%s: ",messageType);
			if(messagefmt!=NULL)written+=vfprintf(messageStream,messagefmt,args); // MDH@13MAR2020: echo to the output file if the flag tells us to
			fflush(messageStream);
		}
		if(written>0)result++;
  	va_end(args);
		messageStream=messageStream->next;
	}
	return result;
}

char const * const M_SYSTEM_ERROR_PREFIX="SYSTEM";
void outputSystemError(char const * const systemError){
	output("%s%s\n",M_SYSTEM_ERROR_PREFIX,systemError);
}

// helper functions to manage the message (type) lists
/*
typedef struct{
	char *msg;
	struct MessageNode *next;
}MessageNode;
*/
typedef struct MessageId{
	char* msgId;
	struct MessageId* next;
}MessageId;
struct MessageIdStack{
	MessageId* first;
	MessageId* last;
}messageIdStack;
/**
 * @brief registers \p messageId as he current (active) message id
 * @param messageId 
 */
char* setMessageId(char const * const messageId){
	if(messageId!=NULL){
		if(messageIdStack.last==NULL||strcmp(messageIdStack.last->msgId,messageId)!=0){
			MessageId* newMessageId=calloc(1,sizeof(MessageId));
			if(newMessageId!=NULL){
				newMessageId->msgId=strdup(messageId); // duplicate the message id
				if(newMessageId->msgId!=NULL){
					if(NULL==messageIdStack.last)
						messageIdStack.first=newMessageId;
					else
						messageIdStack.last->next=newMessageId;
					messageIdStack.last=newMessageId;
				}else{
					free(newMessageId);
					outputSystemError("Failed to register a message id!");
				}
			}else
				outputSystemError("Failed to create a message id!");
		}
	}
	return(messageIdStack.last!=NULL?messageIdStack.last->msgId:NULL);
}
/**
 * @brief the list containing the message lists of a given type
 * 
 */
typedef struct MessageNode{
	Message* message;
	struct MessageNode* next;
}MessageNode;
typedef struct MessageTypeListNode{
	char* messageType;
	size_t count; // keeps track of the number of messages in the message type list
	MessageNode* firstMessageNode; // points to the first message in the queue
	MessageNode* lastMessageNode; // points to the last message in the queue
	struct MessageTypeListNode *next;
}MessageTypeListNode;
// keep a singly-linked list of message type lists
static MessageTypeListNode *firstMessageTypeListNode=NULL,*lastMessageTypeListNode=NULL;
static MessageTypeListNode* getMessageTypeListNode(char const * const messageType){
	MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
	while(messageTypeListNode!=NULL&&strcmp(messageTypeListNode->messageType,messageType)!=0)
		messageTypeListNode=messageTypeListNode->next;
	return messageTypeListNode;
}
static MessageTypeListNode* getNewMessageTypeListNode(char const * const messageType){
	MessageTypeListNode* newMessageTypeListNode=calloc(1,sizeof(MessageTypeListNode));
	if(newMessageTypeListNode!=NULL){
		newMessageTypeListNode->messageType=strdup(messageType);
		if(NULL==newMessageTypeListNode->messageType){
			free(newMessageTypeListNode);
			newMessageTypeListNode=NULL;
		}
	}
	return newMessageTypeListNode;
}
static size_t freedMessageNode(MessageNode* messageNode){
	size_t result=0;
	if(messageNode!=NULL){
		if(messageNode->next!=NULL){result=freedMessageNode(messageNode->next);messageNode->next=NULL;}
		free(messageNode);
		result++;
	}
	return result;
}

static size_t messageIndex=0; // keeps track of the total number of messages
/**
 * @brief returns the message type list node of message type \p messageType
 * 
 * @param messageType 
 * @return MessageTypeListNode* the message type list node of message type \p messageType
 */
static MessageTypeListNode* _getMessageTypeListNode(char const * const messageType){
	MessageTypeListNode* messageTypeListNode=NULL;
	if(messageType!=NULL){
		// find the message type list with of the given messageType
		messageTypeListNode=getMessageTypeListNode(messageType);
		// when not found, try to create one
		if(messageTypeListNode==NULL)
			messageTypeListNode=getNewMessageTypeListNode(messageType);
		if(NULL==messageTypeListNode)return NULL;
		if(NULL==firstMessageTypeListNode)
			firstMessageTypeListNode=messageTypeListNode;
		else
			lastMessageTypeListNode->next=messageTypeListNode;
		lastMessageTypeListNode=messageTypeListNode;
	}
	return messageTypeListNode;
}
/**
 * @brief adds \p messageText to the message queue of type \p messageType
 * 
 * @param messageText 
 * @param messageType 
 * @return true on success
 * @return false on failure
 */
bool addMessageOfType(char const * const messageText,char const * const messageType){
	MessageTypeListNode *messageTypeListNode=_getMessageTypeListNode(messageType);
	if(messageTypeListNode!=NULL){
		MessageNode* messageNode=malloc(sizeof(MessageNode));
		if(messageNode!=NULL){
			messageNode->message=malloc(sizeof(Message));
			if(NULL==messageNode->message){
				free(messageNode);
				return false;
			}
			messageNode->message->msg=strdup(messageText);
			if(NULL==messageNode->message->msg){
				free(messageNode->message);
				free(messageNode);
				return false;
			}
			// MDH@11AUG2024: register the current (active) message id as the id of the new message added
			messageNode->message->id=(messageIdStack.last!=NULL?messageIdStack.last->msgId:NULL);
			messageNode->message->index=++messageIndex;
			messageTypeListNode->count++;
			if(messageTypeListNode->lastMessageNode!=NULL)
				messageTypeListNode->lastMessageNode->next=messageNode;
			messageTypeListNode->lastMessageNode=messageNode;
			if(NULL==messageTypeListNode->firstMessageNode)
				messageTypeListNode->firstMessageNode=messageNode;
		}
	}else
		outputSystemError("Failed to register a message");
	return false;
}
// end message type lists helper functions

/**
 * @brief returns all registered messages in the original order
 * 
 * @return Messages* 
 */
Messages* getMessages(){
	Messages* messages=calloc(1,sizeof(Messages));
	if(messages!=NULL){
		// now we're going to count all messages we need
		size_t totalMessageCount=0,totalMessageTypeCount=0;
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		while(messageTypeListNode!=NULL){
			if(messageTypeListNode->count){
				totalMessageTypeCount++;
				totalMessageCount+=messageTypeListNode->count;
			}
			messageTypeListNode=messageTypeListNode->next;
		}
		if(totalMessageTypeCount){
			MessageNode** messageTypeNodes=calloc(totalMessageTypeCount,sizeof(MessageNode*));
			if(messageTypeNodes!=NULL){
				messages->count=totalMessageCount;
				messages->messages=calloc(totalMessageCount,sizeof(Message*));
				if(messages->messages!=NULL){
					// now we need to merge messages from all the message type list nodes
					// 1. initialize messageTypeListNodes to the first of all the message type list nodes
					MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
					size_t messageTypeIndex=0;
					while(messageTypeListNode!=NULL){
						messageTypeNodes[messageTypeIndex++]=messageTypeListNode->firstMessageNode;
						messageTypeListNode=messageTypeListNode->next;
					}
					// 2. now ready for merging
					size_t messageIndex=0,unfinishedMessageTypeListNodeCount=totalMessageTypeCount;
					while(unfinishedMessageTypeListNodeCount){
						// there's at least one message type list node unequal to NULL
						size_t firstMessageTypeIndex=0;
						while(NULL==messageTypeNodes[firstMessageTypeIndex])firstMessageTypeIndex++;
						// iterate over the remaining ones
						for(int messageTypeIndex=firstMessageTypeIndex+1;messageTypeIndex<totalMessageTypeCount;messageTypeIndex++)
							if(messageTypeNodes[messageTypeIndex]->message->index<messageTypeNodes[firstMessageTypeIndex]->message->index)
								firstMessageTypeIndex=messageTypeIndex;
						// register the message at firstMessageTypeIndex as the next one
						messages->messages[messageIndex++]=messageTypeNodes[firstMessageTypeIndex]->message;
						messageTypeNodes[firstMessageTypeIndex]=messageTypeNodes[firstMessageTypeIndex]->next;
						if(NULL==messageTypeNodes[firstMessageTypeIndex])unfinishedMessageTypeListNodeCount--;
					}
				}
				free(messageTypeNodes);			
			}
			if(messages->messages!=NULL)
				return messages;
		}
		free(messages);
	}
	return NULL;
}

/**
 * @brief returns a MessageList containing all messages of type \p messageType
 * 
 * @param messageType 
 * @return MessageList* the list of collected messages of type \p messageType
 */
Messages* getMessagesOfType(char const * const messageType){
	if(messageType!=NULL){
		MessageTypeListNode* messageTypeListNode=getMessageTypeListNode(messageType);
		if(messageTypeListNode!=NULL){
			Messages* messages=calloc(1,sizeof(Messages));
			if(messages!=NULL){
				messages->messages=calloc(messages->count,sizeof(Message*));
				if(messages->messages!=NULL){
					MessageNode* messageNode=messageTypeListNode->firstMessageNode;
					size_t messageNodeIndex=0;
					while(messageNode!=NULL){
						messages->messages[messageNodeIndex]=messageNode->message;
						messageNodeIndex++;
						if(messageNodeIndex>=messages->count)break;
						messageNode=messageNode->next;
					}
					return messages;
				}
				free(messages);
			}
		}
	}
	return NULL;
}
/**
 * @brief removes all messages of type messageType
 * 
 * @param messageType 
 * @return * exposes 
 */
size_t removeMessagesOfType(char const * const messageType){
	MessageTypeListNode* messageTypeListNode=getMessageTypeListNode(messageType);
	if(messageTypeListNode!=NULL&&messageTypeListNode->count>0){
		size_t freedMessageNodes=freedMessageNode(messageTypeListNode->firstMessageNode);
		messageTypeListNode->count-=freedMessageNodes;
		if(messageTypeListNode->count==0){
			messageTypeListNode->firstMessageNode=NULL;
			messageTypeListNode->lastMessageNode=NULL;
		}
		return messageTypeListNode->count; // return the number of not freed message nodes
	}
	return 0;
}

// MDH@06AUG2024: what if we pass all output through outputf() instead of directly through output() so we can process it
static char* outputText=NULL; // where we're going to collect the output texts
static size_t outputLength,outputSize; // the part currently occupied of outputText
static size_t errorPrefixLength,bugPrefixLength,warningPrefixLength;
char **warnings=NULL,**errors=NULL,**bugs=NULL;
static void registerWarning(){
	if(!addMessageOfType(M_WARNING_PREFIX,outputText+warningPrefixLength))
		outputSystemError("Failed to register a warning!");
}
static void registerError(){
	if(!addMessageOfType(M_ERROR_PREFIX,outputText+errorPrefixLength))
		outputSystemError("Failed to register an error!");
}
static void registerBug(){
	if(!addMessageOfType(M_BUG_PREFIX,outputText+bugPrefixLength))
		outputSystemError("Failed to register a bug!");
}
/**
 * @brief outputs (when \p echoToOutput is true) and collects outputText until \p newlinePosition
 * 
 * @param newlinePosition 
 * @param echoToOutput outputs outputText until newlinePosition when true
 */
static void collectLine(char* newlinePosition,bool echoToOutput){
	assert(newlinePosition);
	*newlinePosition='\0';
	if(echoToOutput)output("%s%c",outputText,'\n');
	// now we can check whether outputText is an error, bug or warning
	if(strncmp(M_ERROR_PREFIX,outputText,errorPrefixLength)==0){
		registerError();
	}else
	if(strncmp(M_BUG_PREFIX,outputText,bugPrefixLength)==0){
		registerBug();
	}else
	if(strncmp(M_WARNING_PREFIX,outputText,warningPrefixLength)==0){
		registerWarning();
	}
	ssize_t shortened=(newlinePosition-outputText);
	if(shortened<=outputLength){
		outputLength-=shortened;
		if(outputLength)memmove(outputText,newlinePosition+1,outputLength);
		// we have to move the remaining text up
		*(outputText+outputLength)='\0';
	}else
		printf("%sCan't shorten the length of the output buffer (%zu) by %zu.\n",M_ERROR_PREFIX,outputLength,shortened);
}

/**
 * @brief outputs \p info prefixed with M_INFO_PREFIX, appending a period when not present in info
 * 
 * @param info the info text to output
 * @returns the number of characters output
 */
size_t outputInfo(char const * const info){
	size_t result=0;
	if(info!=NULL){
		size_t l=strlen(info);
		if(l>0){
			if(NULL==M_INFO_PREFIX)result=q2outputandcollect("%s",info);else
    	result=q2outputandcollect("%s%s",M_INFO_PREFIX,info);
    	l--;if(l>0)if(info[l]!='.'&&info[l]!='!'&&info[l]!='?')result+=q2outputandcollect("%c",'.');// replacing: outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
	    result+=q2outputandcollect("%c",'\n');// replacing: newline();
		}
	}
	return result;
}

/**
 * @brief outputs \p warning prefixed with M_WARNING_PREFIX, appending a period when not present in info
 * 
 * @param warning the warning text to output
 */
size_t outputWarning(char const * const warning){
	size_t result=0;
	if(warning!=NULL){
    size_t l=strlen(warning);
    if(l>0){
			// MDH@19AUG2024: it's a nuisance if a warning does not end with a period and we have to add a period
			//                so we can't directly call addMessageOfType() here
    	if(NULL==M_WARNING_PREFIX)result=q2outputandcollect("%s",warning);else
    	result=q2outputandcollect("%s%s",M_WARNING_PREFIX,warning);
    	l--;if(l>0)if(warning[l]!='.'&&warning[l]!='!'&&warning[l]!='?')result+=q2outputandcollect("%c",'.'); // replacing: outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    	result+=q2outputandcollect("%c",'\n'); // replacing: newline();
		}
	}
	return result;
}

/**
 * @brief outputs \p error prefixed with M_ERROR_PREFIX, appending a period if not present in \p error
 * 
 * @param error the error text to output
 */
size_t outputError(char const * const error){
	size_t result=0;
	if(error!=NULL){
  	size_t l=strlen(error);
  	if(l>0){
  		if(NULL==M_ERROR_PREFIX)result=q2outputandcollect("%s",error);else
    	result=q2outputandcollect("%s%s",M_ERROR_PREFIX,error);
    	l--;if(l>0)if(error[l]!='.'&&error[l]!='!'&&error[l]!='?')result+=q2outputandcollect("%c",'.'); // replacing: outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    	result+=q2outputandcollect("%c",'\n'); // replacing: newline();
		}
	}
	return result;
}

/**
 * @brief outputs \p memoryerror prefixed by a memory error text
 * 
 * @param memoryerror the memory error text
 */
size_t outputMemoryError(char const * const memoryerror){
  if(NULL==memoryerror)return 0;
	return q2outputandcollect("%s%s. Probable cause: out of memory!\n",M_ERROR_PREFIX,memoryerror);
}

// MDH@05NOV2019: might come in handy to be able to report bugs
/**
 * @brief outputs \p error as error and \p text as is
 * 
 * @param error the error text
 * @param text text to output after the error text
 */
size_t outputErrorAndText(char const * const error,char const * const text){
	size_t result=0;
	if(error!=NULL)result=q2outputandcollect("%s%s",M_ERROR_PREFIX,error);
	if(text!=NULL)result+=q2outputandcollect(text);
	return result+q2outputandcollect(".\n");
}

/**
 * @brief outputs \p bug prefixed by M_BUG_PREFIX, postfixing a period if not present in \p bug
 * 
 * @param bug the bug text
 */
size_t outputBug(char const * const bug){
	size_t result=0;
	if(bug!=NULL){
    size_t l=strlen(bug);
		if(l>0){
			if(NULL==M_BUG_PREFIX)result=q2outputandcollect("%s",bug);else
			result=q2outputandcollect("%s%s",M_BUG_PREFIX,bug);
			l--;if(l>0)if(bug[l]!='.'&&bug[l]!='!'&&bug[l]!='?')result+=q2outputandcollect("%c",'.');// replacing: outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
			result+=q2outputandcollect("%c",'\n'); //newline();
		}
	}
	return result;
} 

// for now placing kbhit() here
/**
 * @brief checks the console for a recent keystroke
 * 
 * @return int nonzero when there is a key in the keyboard buffer
 */
int kbhit(){
	struct timeval tv={0L,0L};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1,&fds,NULL,NULL,&tv);
}

/**
 * @brief collects without outputting formatted text
 * 
 * @param fmt 
 * @param ... 
 * @return size_t the number of characters collected
 */
size_t q2collect(char const * const fmt,...){
	size_t result=0;
	if(fmt!=NULL&&strlen(fmt)>0){
		if(outputText!=NULL){
			do{
			  va_list args;
  			va_start(args,fmt);
			 	int count=vsnprintf(outputText+outputLength,outputSize-outputLength,fmt,args);
				va_end(args);
				if(count<=0){
					outputText[outputLength]='\0'; // just in case
					output("%s%s",M_ERROR_PREFIX,"Output format error!");
					break;
				}
				if(outputLength+count<outputSize){ // success
					result=count;
					outputLength+=result; // new start
					// if we have a full line output that full line
					printf(outputText);
					char* newlinePosition=strchr(outputText,'\n');
					if(newlinePosition!=NULL)
						collectLine(newlinePosition,false);
					break;
				}
				outputSize+=64;
				char* newOutputText=realloc(outputText,sizeof(char)*outputSize);
				if(NULL==newOutputText){
					output("%s%s",M_ERROR_PREFIX,"Output memory error");
					break;
				}
				outputText=newOutputText;
			}while(true);
		}else{ // directly pass along to output
		  va_list args;
  		va_start(args,fmt);
			vprintf(fmt,args);
			va_end(args);
		}
	}
	return result;
}

/**
 * @brief logs formatted text
 * 
 * @param fmt 
 * @param ... 
 * @return size_t the number of characters logged
 */
size_t q2outputandcollect(char const * const fmt,...){
	size_t result=0;
	if(fmt!=NULL&&strlen(fmt)>0){
		if(outputText!=NULL){
			do{
			  va_list args;
  			va_start(args,fmt);
			 	int count=vsnprintf(outputText+outputLength,outputSize-outputLength,fmt,args);
				va_end(args);
				if(count<=0){
					outputText[outputLength]='\0'; // just in case
					output("%s%s",M_ERROR_PREFIX,"Output format error!");
					break;
				}
				if(outputLength+count<outputSize){ // success
					result=count;
					outputLength+=result; // new start
					// if we have a full line output that full line
					printf(outputText);
					char* newlinePosition=strchr(outputText,'\n');
					if(newlinePosition!=NULL)
						collectLine(newlinePosition,true);
					break;
				}
				outputSize+=64;
				char* newOutputText=realloc(outputText,sizeof(char)*outputSize);
				if(NULL==newOutputText){
					output("%s%s",M_ERROR_PREFIX,"Output memory error");
					break;
				}
				outputText=newOutputText;
			}while(true);
		}else{ // directly pass along to output
		  va_list args;
  		va_start(args,fmt);
			vprintf(fmt,args);
			va_end(args);
		}
	}
	return result;
}
/**
 * @brief outputs and collects a new line character
 * 
 * @return size_t 
 */
size_t q2newline(){
	return q2outputandcollect("%c",'\n');
}

bool outputCollectorInitialized(){
	messageIdStack.first=NULL;messageIdStack.last=NULL; // MDH@11AUG2024: intialize the message id stack
	outputLength=0;outputSize=256;
	errorPrefixLength=strlen(M_ERROR_PREFIX);
	bugPrefixLength=strlen(M_BUG_PREFIX);
	warningPrefixLength=strlen(M_WARNING_PREFIX);
	outputText=calloc(256,sizeof(char)); // should suffice
	return(outputText!=NULL);
}

/**
 * @brief initializes the output message stream service to always writing message to stdout
 * 
 * @param source the name of the message stream that outputs to stdout
 * @return true on success
 * @return false on failure
 ///
bool messageStreamsInitialized(char const * const source){
	if(initializeOutputCollector())
		output("Text output initialized!\n");
	else
		output("%sFailed to initialize text output.\n",M_ERROR_PREFIX);
	return(_messageStreamStack!=NULL||pushMessageStream(stdout,(source!=NULL?source:""),NULL)); // stdout is the principal output message stream
}
*/

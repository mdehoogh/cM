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
/////extern const char* const M_RESULT_PREFIX="RESULT: "; // MDH@28AUG2024: for results

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
	size_t filtered; // MDH@30AUG2024: this is a temporary field used by _getFilteredMessages() representing the number of messages to return
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
		newMessageTypeListNode->next=NULL; // should not be required though!!!
		newMessageTypeListNode->messageType=strdup(messageType);
		if(NULL==newMessageTypeListNode->messageType){
			free(newMessageTypeListNode);
			newMessageTypeListNode=NULL;
		}
	}
	return newMessageTypeListNode;
}
/*
static size_t freedMessageNode(MessageNode* messageNode){
	size_t result=0;
	if(messageNode!=NULL){
		if(messageNode->next!=NULL){result=freedMessageNode(messageNode->next);messageNode->next=NULL;}
		if(messageNode->message!=NULL){
			////free(messageNode->message->id); // the id points to a registered message id (so message doesn't own it)
			if(messageNode->message->msg!=NULL){
				output("Releasing message '%s'.\n",messageNode->message->msg);
				free(messageNode->message->msg);
			}
			free(messageNode->message);
		}
		free(messageNode);
		result++;
	}
	return result;
}
*/
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
		if(messageTypeListNode==NULL){
			messageTypeListNode=getNewMessageTypeListNode(messageType);
			if(NULL==messageTypeListNode)return NULL;
			if(NULL==firstMessageTypeListNode)
				firstMessageTypeListNode=messageTypeListNode;
			else
				lastMessageTypeListNode->next=messageTypeListNode;
			lastMessageTypeListNode=messageTypeListNode;
		}
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
		///output("Registering message '%s' of type '%s'.\n",messageText,messageType);
		MessageNode* messageNode=calloc(1,sizeof(MessageNode));
		if(messageNode!=NULL){
			//// malloc() above changed to calloc()!!! messageNode->next=NULL; // essential bro'
			messageNode->message=calloc(1,sizeof(Message));
			if(NULL==messageNode->message){
				free(messageNode);
				output("%sFailed to allocate message.\n",M_ERROR_PREFIX);
				return false;
			}
			messageNode->message->msg=strdup(messageText);
			if(NULL==messageNode->message->msg){
				output("%sFailed to store message.\n",M_ERROR_PREFIX);
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
			///output("Message #%zu registered!\n",messageIndex);
			return true;
		}
	}else
		output("%sFailed to register a message.\n",M_ERROR_PREFIX);
	return false;
}
// end message type lists helper functions

/* _getMessagesOfType() can take care of the general case of retrieving all messages as well now
///**
 * @brief returns all registered messages in the original order
 * 
 * @return Messages* 
 
Messages* _getMessages(){
	///output("Retrieving messages.\n");
	Messages* messages=calloc(1,sizeof(Messages));
	if(messages!=NULL){
		// now we're going to count all messages we need
		///output("Counting messages.\n");
		size_t totalMessageCount=0,totalMessageTypeCount=0;
		if(firstMessageTypeListNode!=NULL){
			MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
			while(messageTypeListNode!=NULL){
				if(messageTypeListNode->firstMessageNode!=NULL){ // better test than ->count
					totalMessageTypeCount++;
					totalMessageCount+=messageTypeListNode->count;
				}else
					output("%sNo messages of type '%s'.\n",M_WARNING_PREFIX,messageTypeListNode->messageType);
				messageTypeListNode=messageTypeListNode->next;
			}
		}else
			output("%sNo message types!\n",M_WARNING_PREFIX);
		///output("Messages: types=%zu - count=%zu.\n",totalMessageTypeCount,totalMessageCount);
		if(totalMessageCount>0){
			MessageNode** messageTypeNodes=calloc(totalMessageTypeCount,sizeof(MessageNode*));
			char** messageTypes=calloc(totalMessageTypeCount,sizeof(char*));
			if(messageTypeNodes!=NULL&&messageTypes!=NULL){
				messages->count=totalMessageCount;
				messages->messages=calloc(totalMessageCount,sizeof(Message*));
				messages->types=calloc(totalMessageCount,sizeof(char*)); // allocate the types
				if(messages->messages!=NULL&&messages->types!=NULL){
					///output("Collecting messages.\n");
					// now we need to merge messages from all the message type list nodes
					// 1. initialize messageTypeListNodes to the first of all the message type list nodes
					MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
					size_t messageTypeIndex=0;
					while(messageTypeListNode!=NULL){
						messageTypeNodes[messageTypeIndex]=messageTypeListNode->firstMessageNode;
						messageTypes[messageTypeIndex]=messageTypeListNode->messageType;
						messageTypeIndex++;
						messageTypeListNode=messageTypeListNode->next;
					}
					// 2. now ready for merging
					size_t collectedMessageIndex=0,unfinishedMessageTypeListNodeCount=totalMessageTypeCount;
					///output("Number of unfinished message types: %zu.\n",unfinishedMessageTypeListNodeCount);
					while(unfinishedMessageTypeListNodeCount>0&&collectedMessageIndex<totalMessageCount){
						///output("Collecting message #%zu.\n",collectedMessageIndex+1);
						// there's at least one message type list node unequal to NULL
						long long firstMessageTypeIndex=-1,messageTypeIndex=totalMessageTypeCount;
						while(--messageTypeIndex>=0)
							if(messageTypeNodes[messageTypeIndex]!=NULL
									&&(firstMessageTypeIndex<0
										||messageTypeNodes[messageTypeIndex]->message->index<messageTypeNodes[firstMessageTypeIndex]->message->index))
								firstMessageTypeIndex=messageTypeIndex;
						if(firstMessageTypeIndex<0){outputBug("No messages left!");break;} // should not happen!!
						// register the message at firstMessageTypeIndex as the next one
						messages->messages[collectedMessageIndex]=messageTypeNodes[firstMessageTypeIndex]->message;
						messages->types[collectedMessageIndex]=messageTypes[firstMessageTypeIndex];
						collectedMessageIndex++;
						///output("Message #%zu of type #%zu collected.\n",collectedMessageIndex,firstMessageTypeIndex);
						messageTypeNodes[firstMessageTypeIndex]=messageTypeNodes[firstMessageTypeIndex]->next;
						if(NULL==messageTypeNodes[firstMessageTypeIndex]){
							unfinishedMessageTypeListNodeCount--;
							///output("Number of message types left: %zu.\n",unfinishedMessageTypeListNodeCount);
						}
					}
				}else{
					if(messages->types!=NULL)free(messages->types);
					if(messages->messages!=NULL)free(messages->messages);
					output("%sNo memory for messages.\n",M_ERROR_PREFIX);
				}
				//////free(messageTypeNodes);			
			}else{
				////////free(messageTypeNodes);free(messageTypes);
				output("%sNo message type nodes.\n",M_WARNING_PREFIX);
			}
			// free() is ok with passing NULLs in as well
			free(messageTypeNodes);
			free(messageTypes);
			if(messages->messages!=NULL&&messages->types!=NULL)
				return messages;
		}else
			output("%sNo messages.\n",M_WARNING_PREFIX);
		if(messages->messages!=NULL)free(messages->messages); // unlikely though
		if(messages->types!=NULL)free(messages->types);
		free(messages);
	}else
		output("%sNo memory for messages array!\n",M_WARNING_PREFIX);
	return NULL;
}
*/
/**
 * @brief frees \p messages returned by _getMessages() or _getMessagesOfType()
 * 
 * @param messages the messages to free
 */
void freeMessages(Messages* messages){
	if(NULL==messages)return;
	///output("Freeing messages.\n");
	if(messages->messages!=NULL)free(messages->messages);
	if(messages->types!=NULL)free(messages->types);
	free(messages);
}

/**
 * @brief outputs \p messageCounts
 * 
 * @param messageCounts 
 */
static void outputMessageCounts(MessageCounts const * const messageCounts){
	if(messageCounts!=NULL){
		output("Outputting %zu message counts.\n",messageCounts->count);
		if(messageCounts->count>0){
			for(size_t messageCountIndex=0;messageCountIndex<messageCounts->count;messageCountIndex++)
				output("\t%zu. %s:%zu.\n",messageCountIndex+1
				,messageCounts->messagecounts[messageCountIndex].messageType,messageCounts->messagecounts[messageCountIndex].count);
		}
	}else
		output("%sNo message counts defined to output!\n",M_ERROR_PREFIX);
}
/**
 * @brief frees \p message
 * 
 * @param message 
 */
static void freeMessage(Message* message){
	if(NULL==message)return;
	if(message->msg!=NULL)free(message->msg);
	free(message);
}
/**
 * @brief frees \p messageNode
 * 
 * @param messageNode 
 */
static void freeMessageNode(MessageNode* messageNode){
	if(NULL==messageNode)return;
	if(messageNode->message!=NULL)freeMessage(messageNode->message);
	free(messageNode);
}
/**
 * @brief updates the filtered field of all message type list nodes from \p messageCounts
 * 
 * @param messageCounts 
 * @result the number of message types with non-zero filtered fields
 */
static size_t updateMessageTypeListNodeFiltered(MessageCounts const * const messageCounts,size_t* _totalMessageCount){
	*_totalMessageCount=0;
	size_t totalMessageTypeCount=0; // the value to return
	MessageCount* messageCountFilters=(messageCounts!=NULL?messageCounts->messagecounts:NULL);
	size_t numberOfFilters=(messageCountFilters!=NULL?messageCounts->count:0);
	if(numberOfFilters>0){ // there are filters, so there may be messages
		size_t messageTypePrefixLength,filterCount;
		// we could collect the message types present in messageCounts?
		// NO, we can simply iterate over the messageCount structures in messageCounts->messagecounts
		// NOTE some message types might not match the filters in which case NONE of the messages are to be passed
		//      where can we actually store 
		char* messageTypePrefix;
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		while(messageTypeListNode!=NULL){
			messageTypeListNode->filtered=0; // assume no filter matches
			// messageTypeCount is the maximum number of messages that match the filters
			size_t messageTypeCount=messageTypeListNode->count; // the maximum number of (returnable) messages of the current type
			if(messageTypeCount>0){ // there are messages of this type that we could return
				// iterate over all filters (which we know there are!!!), and adapt ->filtered accordingly
				size_t filterCountIndex=numberOfFilters;
				do{
					filterCountIndex--;
					// does this message type match this filter???
					messageTypePrefix=messageCountFilters[filterCountIndex].messageType;
					messageTypePrefixLength=strlen(messageTypePrefix);
					if(NULL==messageTypePrefix
							||(messageTypePrefixLength==0&&strlen(messageTypeListNode->messageType)==0)
							||(messageTypePrefixLength>0&&strncmp(messageTypeListNode->messageType,messageTypePrefix,messageTypePrefixLength)==0)){
						// the message type matches the filter message type
						// there's only a restriction if the filter count is positive and below messageTypeCunt
						filterCount=messageCountFilters[filterCountIndex].count; // the maximum number of messages to return
						// if filterCount==0 all messages of this type are allowed, so does not limit filtered!!!!!
						// but we should NOT break, because other filters should still be allowed to limit the number of message of this type to return
						if(filterCount==0){ // all messages in this type should be returned
							if(messageTypeListNode->filtered==0)
								messageTypeListNode->filtered=(messageTypeCount=messageTypeListNode->count);
						}else
						// only to be returned partly when filterCount is below the current message type count
						if(filterCount<messageTypeCount) // more restrictive
							messageTypeListNode->filtered=(messageTypeCount=filterCount);
					}
				}while(filterCountIndex);
				if(messageTypeListNode->filtered){
					*_totalMessageCount+=messageTypeListNode->filtered;
					totalMessageTypeCount++; // register as one of the returned message types
				}
			}
			// the next message type list to check
			messageTypeListNode=messageTypeListNode->next;
		}
	}else
	if(NULL==messageCounts){ // force all messages to be returned
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		while(messageTypeListNode!=NULL){
			messageTypeListNode->filtered=messageTypeListNode->count;
			*_totalMessageCount+=messageTypeListNode->filtered;
			totalMessageTypeCount++;
			messageTypeListNode=messageTypeListNode->next;
		}
	}
	return totalMessageTypeCount;
}
/**
 * @brief returns the messages filtered by \p messageCounts
 * 
 * @param messageCounts the original message counts
 * @return Messages* the messages filtered by \p messageCounts
 */
Messages* _getFilteredMessages(MessageCounts * const messageCounts){
	outputMessageCounts(messageCounts);
	Messages* _messages=calloc(1,sizeof(Messages));
	if(_messages!=NULL){
		// we want to store for each message type how many messages to return
		// we could well store this amount with the message type itself
		// 1. count all the messages
		size_t totalMessageCount=0;
		size_t totalMessageTypeCount=updateMessageTypeListNodeFiltered(messageCounts,&totalMessageCount);
		// if there are NO messages to return, just return _messages as is
		if(totalMessageCount==0)return _messages;
		output("Total number of matching messages: %zu.\n",totalMessageCount);
		// there are messages to return
		// we need to start out from the right message nodes
		MessageNode** messageTypeNodes=calloc(totalMessageTypeCount,sizeof(MessageNode*));
		char** messageTypes=calloc(totalMessageTypeCount,sizeof(char*));
		if(messageTypeNodes!=NULL&&messageTypes!=NULL){
			///output("Total number of matching messages: %zu.\n",totalMessageCount);
			_messages->messages=calloc(totalMessageCount,sizeof(Message*));
			_messages->types=calloc(totalMessageCount,sizeof(char*));
			if(_messages->messages!=NULL&&_messages->types!=NULL){
				_messages->count=totalMessageCount;
				///output("Collecting messages.\n");
				// now we need to merge messages from all the message type list nodes
				// 1. initialize messageTypeListNodes to the first of all the message type list nodes
				MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
				size_t messageTypeIndex=0,messagesNotToReturn;
				MessageNode* firstMessageTypeNode; // this is the first message node for a given type to return
				while(messageTypeListNode!=NULL){
					if(messageTypeListNode->filtered){ // messages of this type are to be returned
						messagesNotToReturn=messageTypeListNode->count-messageTypeListNode->filtered;
						firstMessageTypeNode=messageTypeListNode->firstMessageNode;
						while(firstMessageTypeNode!=NULL&&messagesNotToReturn>0){
							firstMessageTypeNode=firstMessageTypeNode->next;
							messagesNotToReturn--;
						}
						if(firstMessageTypeNode!=NULL){ // messages left to return
							messageTypeNodes[messageTypeIndex]=firstMessageTypeNode;
							messageTypes[messageTypeIndex]=messageTypeListNode->messageType;
							messageTypeIndex++;
						}
					}
					messageTypeListNode=messageTypeListNode->next;
				}
				// 2. now ready for merging
				size_t collectedMessageIndex=0,unfinishedMessageTypeListNodeCount=totalMessageTypeCount;
				///output("Number of unfinished message types: %zu.\n",unfinishedMessageTypeListNodeCount);
				while(unfinishedMessageTypeListNodeCount>0&&collectedMessageIndex<totalMessageCount){
					///output("Collecting message #%zu.\n",collectedMessageIndex+1);
					// there's at least one message type list node unequal to NULL
					long long firstMessageTypeIndex=-1,messageTypeIndex=totalMessageTypeCount;
					while(--messageTypeIndex>=0)
						if(messageTypeNodes[messageTypeIndex]!=NULL
								&&(firstMessageTypeIndex<0
									||messageTypeNodes[messageTypeIndex]->message->index<messageTypeNodes[firstMessageTypeIndex]->message->index))
							firstMessageTypeIndex=messageTypeIndex;
					if(firstMessageTypeIndex<0){outputBug("No messages left!");break;} // should not happen!!
					// register the message at firstMessageTypeIndex as the next one
					_messages->messages[collectedMessageIndex]=messageTypeNodes[firstMessageTypeIndex]->message;
					_messages->types[collectedMessageIndex]=messageTypes[firstMessageTypeIndex];
					collectedMessageIndex++;
					///output("Message #%zu of type #%zu collected.\n",collectedMessageIndex,firstMessageTypeIndex);
					messageTypeNodes[firstMessageTypeIndex]=messageTypeNodes[firstMessageTypeIndex]->next;
					if(NULL==messageTypeNodes[firstMessageTypeIndex]){
						unfinishedMessageTypeListNodeCount--;
						///output("Number of message types left: %zu.\n",unfinishedMessageTypeListNodeCount);
					}
				}
			}
			/* replacing:
			size_t messageNodeIndex=0;
			messageTypeListNode=firstMessageTypeListNode;
			size_t messagesToReturn,messagesNotToReturn;
			while(messageTypeListNode!=NULL){
				messagesToReturn=messageTypeListNode->filtered;
				if(messagesToReturn>0){ // we are to return this amount of messages
					char* messageType=messageTypeListNode->messageType;
					messagesNotToReturn=messageTypeListNode->count-messagesToReturn;
					///output("Adding %zu messages of type '%s'.\n",messageTypeListNode->count,messageType);
					MessageNode* messageNode=messageTypeListNode->firstMessageNode;
					while(messagesNotToReturn>0){
						messageNode=messageNode->next;
						messagesNotToReturn--;
					}
					while(messageNode!=NULL){
						///output("Adding message #%zu.\n",messageNodeIndex+1);
						_messages->messages[messageNodeIndex]=messageNode->message;
						_messages->types[messageNodeIndex]=messageType;
						messageNodeIndex++;
						///output("Message #%zu added.\n",messageNodeIndex);
						if(messageNodeIndex>=totalMessageCount)break;
						messageNode=messageNode->next;
					}
				}
				if(messageNodeIndex>=totalMessageCount)break;
				messageTypeListNode=messageTypeListNode->next;
			}
			*/
			///output("Messages retrieved.\n");
			return _messages;
		}
		output("Failed to allocate memory to store messages.\n",M_ERROR_PREFIX);
		freeMessages(_messages);
	}
	return NULL;
}
/* replaces:
// * @brief returns a MessageList containing all messages of type \p messageType
// * 
// * @param messageType 
// * @return MessageList* the list of collected messages of type \p messageType
// /
Messages* _getMessagesOfType(char const * const messageTypePrefix){
	Messages* messages=calloc(1,sizeof(Messages));
	if(messages!=NULL){
		size_t messageTypePrefixLength=(messageTypePrefix!=NULL?strlen(messageTypePrefix):0);
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		size_t totalMessageCount=0;
		while(messageTypeListNode!=NULL){
			if(NULL==messageTypePrefix
				||(messageTypePrefixLength==0
					?strlen(messageTypeListNode->messageType)==0
					:strncmp(messageTypeListNode->messageType,messageTypePrefix,messageTypePrefixLength)==0))
				totalMessageCount+=messageTypeListNode->count;
			messageTypeListNode=messageTypeListNode->next;
		}
		if(totalMessageCount>0){
			///output("Total number of matching messages: %zu.\n",totalMessageCount);
			// we need to count the messages
			messages->messages=calloc(totalMessageCount,sizeof(Message*));
			messages->types=calloc(totalMessageCount,sizeof(char*));
			if(messages->messages!=NULL&&messages->types!=NULL){
				messages->count=totalMessageCount;
				size_t messageNodeIndex=0;
				messageTypeListNode=firstMessageTypeListNode;
				while(messageTypeListNode!=NULL){
					///output("Checking %zu messages of type '%s'.\n",messageTypeListNode->count,messageTypeListNode->messageType);
					if(NULL==messageTypePrefix
						||(messageTypePrefixLength==0&&strlen(messageTypeListNode->messageType)==0)
						||(messageTypePrefixLength>0&&strncmp(messageTypeListNode->messageType,messageTypePrefix,messageTypePrefixLength)==0)){
						char* messageType=messageTypeListNode->messageType;
						///output("Adding %zu messages of type '%s'.\n",messageTypeListNode->count,messageType);
						MessageNode* messageNode=messageTypeListNode->firstMessageNode;
						while(messageNode!=NULL){
							///output("Adding message #%zu.\n",messageNodeIndex+1);
							messages->messages[messageNodeIndex]=messageNode->message;
							messages->types[messageNodeIndex]=messageType;
							messageNodeIndex++;
							///output("Message #%zu added.\n",messageNodeIndex);
							if(messageNodeIndex>=totalMessageCount)break;
							messageNode=messageNode->next;
						}
					}
					if(messageNodeIndex>=totalMessageCount)break;
					messageTypeListNode=messageTypeListNode->next;
				}
				///output("Messages retrieved.\n");
				return messages;
			}
			output("Failed to allocate memory to store messages.\n",M_ERROR_PREFIX);
		}
		free_messages(messages);
	}
	return NULL;
}
*/

/**
 * @brief removes all messages registered in \p messageTypeListNode
 * 
 * @param messageTypeListNode 
 */
static void removeFilteredFromMessageTypeListNode(MessageTypeListNode * const messageTypeListNode){
	if(NULL==messageTypeListNode)return; // no type to remove from
	///output("Removing %zu message%s of type '%s'.\n",messageTypeListNode->count,(messageTypeListNode->count>1?"s":""),messageTypeListNode->messageType);
	if(messageTypeListNode->count==0)return; // nothing left to remove
	size_t messagesToRemove=messageTypeListNode->filtered; // number of messages to remove
	if(messagesToRemove==0)return; // nothing requested to remove
	MessageNode *messageNode,*nextMessageNode;
	do{
		messageNode=messageTypeListNode->firstMessageNode;
		if(NULL==messageNode)break; // no first message node to remove
		nextMessageNode=messageNode->next; // remember next to remove
		freeMessageNode(messageNode); // free the first message node
		messageTypeListNode->count--; // one less message in the list
		messageTypeListNode->firstMessageNode=nextMessageNode; // replace the first message node
	}while(--messagesToRemove); // loop until all requested removed
	if(messageTypeListNode->count==0){
		messageTypeListNode->lastMessageNode=NULL;
		if(messageTypeListNode->firstMessageNode!=NULL){
			messageTypeListNode->firstMessageNode=NULL;
			output("%sFailed to remove all messages of type '%s'.\n",M_BUG_PREFIX,messageTypeListNode->messageType);
		}
	}
	/* replacing:
	size_t freedMessageNodes=freedMessageNode(messageTypeListNode->firstMessageNode);
	messageTypeListNode->count-=freedMessageNodes;
	if(messageTypeListNode->count==0){
		messageTypeListNode->firstMessageNode=NULL;
		messageTypeListNode->lastMessageNode=NULL;
	}else
		output("%sNot all messages of type '%s' removed!",M_ERROR_PREFIX,messageTypeListNode->messageType);
		*/
}
/**
 * @brief removes all messages of type messageType
 * 
 * @param messageType 
 * @return * exposes 
 */
long long removeMessages(MessageCounts const * const messageCounts){
	long long unremovedMessageCount=-1;
	size_t totalRemovedMessageCount=0;
	size_t totalRemovedMessageTypeCount=updateMessageTypeListNodeFiltered(messageCounts,&totalRemovedMessageCount);
	if(totalRemovedMessageCount>0){
		unremovedMessageCount=0;
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		while(messageTypeListNode!=NULL){
			// for each type we may determine how many messages we should remove
			// we can store this amount in the filtered field as well like we did in _getFilter
			if(messageTypeListNode->filtered){
				size_t originalCount=messageTypeListNode->count;
				removeFilteredFromMessageTypeListNode(messageTypeListNode);
				unremovedMessageCount+=(messageTypeListNode->count-(originalCount-messageTypeListNode->filtered));
				messageTypeListNode->filtered=0; // not really required
			}
			messageTypeListNode=messageTypeListNode->next;
		}
	}
	return unremovedMessageCount;
}

/**
 * @brief returns the message counts of type \p messageType
 * 
 * @param messageType 
 * @return MessageCounts* 
 */
MessageCounts* _getMessageCounts(){
	MessageCounts* messageCounts=calloc(1,sizeof(MessageCounts));
	if(messageCounts!=NULL){
		size_t totalMessageTypeCount=0;
		MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
		while(messageTypeListNode!=NULL){
			totalMessageTypeCount++;
			messageTypeListNode=messageTypeListNode->next;
		}
		if(totalMessageTypeCount>0){
			messageCounts->messagecounts=calloc(totalMessageTypeCount,sizeof(MessageCount));
			if(messageCounts->messagecounts!=NULL){
				messageCounts->count=totalMessageTypeCount;
				totalMessageTypeCount=0;
				MessageTypeListNode* messageTypeListNode=firstMessageTypeListNode;
				while(messageTypeListNode!=NULL){
					// MDH@30AUG2024: we're strdup()ing messageType here so free_messagecounts() can free the messageType's
					//                because the MessageCounts that getFilteredMessages() receive also has dynamically allocated message types!!!
					// TODO what if strdup() fails????? for now, we just ignore it assuming it won't!!!
					messageCounts->messagecounts[totalMessageTypeCount++]
						=(MessageCount){messageTypeListNode->count,strdup(messageTypeListNode->messageType)};
					messageTypeListNode=messageTypeListNode->next;
				}
				return messageCounts;
			}		
		}
		freeMessageCounts(messageCounts);
	}
	return NULL;
}
/**
 * @brief frees \p messageCounts
 * 
 * @param messageCounts 
 */
void freeMessageCounts(MessageCounts* messageCounts){
	if(NULL==messageCounts)return;
	if(messageCounts->count){
		do{
			messageCounts->count--;
			if(messageCounts->messagecounts[messageCounts->count].messageType!=NULL)
				free(messageCounts->messagecounts[messageCounts->count].messageType);
		}while(messageCounts->count);
	}
	if(messageCounts->messagecounts)free(messageCounts->messagecounts);
	free(messageCounts);
}


// MDH@06AUG2024: what if we pass all output through outputf() instead of directly through output() so we can process it
static char* outputText=NULL; // where we're going to collect the output texts
static size_t outputLength,outputSize; // the part currently occupied of outputText
static size_t errorPrefixLength,bugPrefixLength,warningPrefixLength;
char **warnings=NULL,**errors=NULL,**bugs=NULL;
static void registerWarning(){
	if(!addMessageOfType(outputText+warningPrefixLength,M_WARNING_PREFIX))
		outputSystemError("Failed to register a warning!");
}
static void registerError(){
	if(!addMessageOfType(outputText+errorPrefixLength,M_ERROR_PREFIX))
		outputSystemError("Failed to register an error!");
}
static void registerBug(){
	if(!addMessageOfType(outputText+bugPrefixLength,M_BUG_PREFIX))
		outputSystemError("Failed to register a bug!");
}
static void registerMessage(){
	if(!addMessageOfType(outputText,""))
		outputSystemError("Failed to register a message!");
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
	////////output("Collecting line '%s'.\n",outputText);
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
	}else
		registerMessage();

	// increment newlinePosition so it will stand on the first character of the next line (if any)
	newlinePosition++; 
	ssize_t shortened=(newlinePosition-outputText);
	if(shortened<=outputLength){
		outputLength-=shortened;
		if(outputLength)memmove(outputText,newlinePosition,outputLength);
		// we have to move the remaining text up
		*(outputText+outputLength)='\0';
	}else
		output("%sCan't shorten the length of the output buffer (%zu) by %zu.\n",M_ERROR_PREFIX,outputLength,shortened);
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
	output("Warning length: %zu.\n",result);
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
				///output("Output size: %llu - length: %llu - format: '%s'",outputSize,outputLength,fmt);
			  va_list args;
  			va_start(args,fmt);
			 	int count=vsnprintf(outputText+outputLength,outputSize-outputLength,fmt,args);
				va_end(args);
				///output("Count: %d",count);
				if(count<=0){
					///////outputText[outputLength]='\0'; // just in case
					output("%s%s",M_ERROR_PREFIX,"Output format error!");
					break;
				}
				if(outputLength+count<outputSize){ // success
					result=count;
					outputLength+=result; // new start
					///////outputText[outputLength]='\0'; // just in case
					///output(" - output length: %llu",outputLength);
					// if we have a full line output that full line
					///output("%s"," - output text: <<<<<<<");
					//////output("%s",outputText);
					///for(size_t i=0;i<outputSize&&outputText[i]!=0;i++)output("%c",outputText[i]);
					///output("%s",">>>>>>>>>>\n");
					char* newlinePosition=strchr(outputText,'\n');
					if(newlinePosition!=NULL)
						collectLine(newlinePosition,false);
					///else output("%s!\n","No end-of-line");
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
			output("%sNo output collector!\n",M_WARNING_PREFIX);
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
					//////outputText[outputLength]='\0'; // just in case
					output("%s%s\n",M_ERROR_PREFIX,"Output format error!");
					break;
				}
				if(outputLength+count<outputSize){ // success
					result=count;
					outputLength+=result; // new start
					// if we have a full line output that full line
					////////printf("%s",outputText);
					char* newlinePosition=strchr(outputText,'\n');
					if(newlinePosition!=NULL)
						collectLine(newlinePosition,true);
					break;
				}
				outputSize+=64;
				char* newOutputText=realloc(outputText,sizeof(char)*outputSize);
				if(NULL==newOutputText){
					output("%s%s\n",M_ERROR_PREFIX,"Output memory error");
					break;
				}
				outputText=newOutputText;
			}while(true);
		}else{ // directly pass along to output
			output("%sNo output collector!\n",M_WARNING_PREFIX);
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
size_t q2newline(bool echoToOutput){
	return(echoToOutput?q2outputandcollect("%c",'\n'):q2collect("%c",'\n'));
}

bool outputCollectorInitialized(){
	messageIdStack.first=NULL;messageIdStack.last=NULL; // MDH@11AUG2024: intialize the message id stack
	errorPrefixLength=strlen(M_ERROR_PREFIX);
	bugPrefixLength=strlen(M_BUG_PREFIX);
	warningPrefixLength=strlen(M_WARNING_PREFIX);
	outputLength=0;outputSize=256;
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

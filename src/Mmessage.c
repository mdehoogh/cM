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

// MDH@06AUG2024: what if we pass all output through outputf() instead of directly through output() so we can process it
static char* outputText=NULL; // where we're going to collect the output texts
static size_t outputLength,outputSize; // the part currently occupied of outputText
static size_t errorPrefixLength,bugPrefixLength,warningPrefixLength;
char **warnings=NULL,**errors=NULL,**bugs=NULL;
static void registerWarning(){

}
static void registerError(){

}
static void registerBug(){

}
static void outputLine(char* newlinePosition){
	*newlinePosition='\0';
	output("%s%c",outputText,'\n');
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
	outputLength-=(newlinePosition-outputText);
	if(outputLength)memmove(outputText,newlinePosition+1,outputLength);
	// we have to move the remaining text up
	*(outputText+outputLength)='\0';
}
static bool initializeTextOutput(){
	errorPrefixLength=strlen(M_ERROR_PREFIX);
	bugPrefixLength=strlen(M_BUG_PREFIX);
	warningPrefixLength=strlen(M_WARNING_PREFIX);
	outputText=calloc(256,sizeof(char)); // should suffice
	if(NULL==outputText)return false;
	outputLength=0;outputSize=256;
	return true;
}
static size_t outputf(char const * const fmt,...){
	size_t result=0;
	if(fmt!=NULL){
	  va_list args;
  	va_start(args,fmt);
		if(outputText!=NULL){
			do{
			 	int count=snprintf(outputText+outputLength,outputSize-outputLength,fmt,args);
				if(count<0){
					output("%s%s",M_ERROR_PREFIX,"Output format error!");
					break;
				}
				if(count>0){
					if(!*(outputText+outputLength+count)){ // success
						result=count;
						outputLength+=result; // new start
						// if we have a full line output that full line
						char* newlinePosition=strchr(outputText,'\n');
						if(newlinePosition!=NULL){
							outputLine(newlinePosition);
						}
						break;
					}else{
						outputSize+=64;
						char* newOutputText=realloc(outputText,sizeof(char)*outputSize);
						if(NULL==newOutputText){
							output("%s%s",M_ERROR_PREFIX,"Output memory error");
							break;
						}
						outputText=newOutputText;
						// and try to fit the text in again
					}
				}
			}while(outputText!=NULL);
		}else // directly pass along to output
			output(fmt,args);
		va_end(args);
	}
	return result;
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
			if(NULL==M_INFO_PREFIX)result=outputf("%s",info);else
    	result=output("%s%s",M_INFO_PREFIX,info);
    	l--;if(l>0)if(info[l]!='.'&&info[l]!='!'&&info[l]!='?')result+=outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
	    result+=newline();
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
    	if(NULL==M_WARNING_PREFIX)result=output("%s",warning);else
    	result=output("%s%s",M_WARNING_PREFIX,warning);
    	l--;if(l>0)if(warning[l]!='.'&&warning[l]!='!'&&warning[l]!='?')result+=outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    	result+=newline();
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
  		if(NULL==M_ERROR_PREFIX)result=output("%s",error);else
    	result=output("%s%s",M_ERROR_PREFIX,error);
    	l--;if(l>0)if(error[l]!='.'&&error[l]!='!'&&error[l]!='?')result+=outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
    	result+=newline();
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
	return output("%s%s. Probable cause: out of memory!\n",M_ERROR_PREFIX,memoryerror);
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
	if(error!=NULL)result=output("%s%s",M_ERROR_PREFIX,error);
	if(text!=NULL)result+=output(text);
	return result+output(".\n");
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
			if(NULL==M_BUG_PREFIX)result=output("%s",bug);else
			result=output("%s%s",M_BUG_PREFIX,bug);
			l--;if(l>0)if(bug[l]!='.'&&bug[l]!='!'&&bug[l]!='?')result+=outputChar('.'); // if the bug doesn't end with a period, exclamation sign or question mark put a period behind it
			result+=newline();
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
size_t outputMessage(char const * const messageType,char const * const messagefmt,...){
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
/**
 * @brief initializes the output message stream service to always writing message to stdout
 * 
 * @param source the name of the message stream that outputs to stdout
 * @return true on success
 * @return false on failure
 */
bool messageStreamsInitialized(char const * const source){
	if(initializeTextOutput())
		output("Text output initialized!\n");
	else
		output("%sFailed to initialize text output.\n",M_ERROR_PREFIX);
	return(_messageStreamStack!=NULL||pushMessageStream(stdout,(source!=NULL?source:""),NULL)); // stdout is the principal output message stream
}
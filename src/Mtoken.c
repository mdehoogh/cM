#include "Mtoken.h"

#include <limits.h>

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TOKEN,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_BUG_PREFIX;

// MDH@11JUN2020: always call free_token on disowned tokens
/**
 * @brief sets the ownership of the M token pointed to by \p _token to \p owner_token
 * 
 * @param _token the pointer to a M token
 * @param owner_token the new owner of the M token
 * @return Mtoken* the owned M token pointer \p _token
 */
Mtoken* owned_token(Mtoken* _token,Mallocationowner owner_token){
	if(!_token)return NULL;
	owned_token(_token->next,owner_token);
	owned_string(_token->text,Msubowner(owner_token,1));
	return OWNED(_token,owner_token);
}
/**
 * @brief disowns the M token pointed to by \p _token from its current owner \p owner_token
 * 
 * @param _token the pointer to the M token to be disowned
 * @param owner_token the current M token owner
 * @return Mtoken* the pointer to the disowned M token
 */
Mtoken* disowned_token(Mtoken* _token,Mallocationowner owner_token){
	if(!_token)return NULL;
	disowned_token(_token->next,owner_token);
	disowned_string(_token->text,Msubowner(owner_token,1));
	return DISOWNED(_token,owner_token);
}
/**
 * @brief frees the M token pointed to by \p _token
 * 
 * @param _token the M token pointer to free
 */
void free_token(Mtoken* _token/*,Mallocationowner owner*/){
	// MDH@19MAY2020: we can free it only when we own it
	if(!_token)return;
	if(_token->next){free_token(_token->next/*,owner*/);_token->next=NULL;}
	if(_token->text){
		if(amVerboseDebugging())output("Freeing token '%s'.\n",string(_token->text));
		free_string(_token->text/*,owner*/);_token->text=NULL;
	}
	FREE_1(_token,'O'/*,owner*/);
}

/**
 * @brief returns a pointer to a new M token initialized to zeroes
 * 
 * @return Mtoken* the pointer to the new M token
 */
Mtoken* __token(){Mallocationowner owner=getOwner(__LINE__);
	return disowned_token(CALLOC_1(sizeof(Mtoken),'O',owner),owner);
}

// MDH@23JUN2020: we might want to change the way we store the number of significant token characters in the future
//                will return a negative value if _token is undefined
/**
 * @brief returns the number of significant characters in the M token pointed to by \p token
 * 
 * @param token the pointer to the M token
 * @return size_t the number of significant characters in the M token on success, or SIZE_T_MAX on failure
 */
size_t getTokenSignificantCharacterCount(Mtoken const * const token){
	if(token)return token->significantCharacterCount;
	q2outputBug("Can't return the number of significant characters of an undefined token.");
	return SIZE_T_MAX; // which is the best value to return to indicate invalid input
}
/**
 * @brief sets the number of significant characters in the M token pointed to by \p token to \p significantCharacterCount
 * 
 * @param token the pointer to the M token
 * @param significantCharacterCount the number of significant characters
 * @return true on success
 * @return false on failure
 */
bool setTokenSignificantCharacterCount(Mtoken * const token,size_t significantCharacterCount){
	if(token){
		token->significantCharacterCount=significantCharacterCount;
		return(token->significantCharacterCount==significantCharacterCount);
	}
	q2outputBug("Can't set the number of significant characters of an undefined token.");
	return false;
}

// a lot of times we're doing the following with the significant character counts
/**
 * @brief returns the significant characters in the M token pointed to by \p token
 * 
 * @param token the pointer to the M token
 * @return char* the significant characters in the M token
 */
char* _getSignificantTokenCharacters(Mtoken const * const token){
	return(token?_stringstart(token->text,token->significantCharacterCount):NULL);
}
/**
 * @brief returns the significant characters in the M token pointed to by \p token as a Mstring pointer
 * 
 * @param token the pointer to the M token
 * @return Mstring* the pointer to a M string wrapping the significant characters of the M token pointed to by \p token
 */
Mstring* _getSignificantTokenText(Mtoken const * const token){
	return(token?_stringCopy(token->text,token->significantCharacterCount):NULL);
}
/**
 * @brief returns the token text stored in the M token pointed to by \p token wrapped in a Mstring
 * 
 * @param token the pointer to the M token
 * @return Mstring* the pointer to a M string wrapping all characters of the M token pointed to by \p token
 */
Mstring* _getTokenText(Mtoken const * const token){
	return(token?_stringCopy(token->text,0):NULL);
}
/**
 * @brief returns whether or not the M token pointed to by \p token is unfinished
 * 
 * @param token the pointer to the M token
 * @return true if the M token is unfinished
 * @return false if the M token is not unfinished
 */
bool isTokenUnfinished(Mtoken const * const token){
	if(token)return(token->significantCharacterCount==0);
	q2outputBug("Can't determine whether an undefined token is unfinished.");
	return false;
}
/**
 * @brief returns whether or not the M token pointed to by \p token is finished
 * 
 * @param token the pointer to the M token
 * @return true if the M token is finished
 * @return false if the M token is not finished
 */
bool isTokenFinished(Mtoken const * const token){
	if(token)return(token->significantCharacterCount>0);
	q2outputBug("Can't determine whether an undefined token is finished.");
	return false;
}
/**
 * @brief finishes the M token pointed to by \p token by setting its number of significant characters to the number of token characters
 * 
 * @param token the pointer to the M token
 */
void finishToken(Mtoken * const token){
	if(token!=NULL){
		token->significantCharacterCount=string_length(token->text);
		logToOutputFile("Finishing token '%s'!\n",string(token->text));
	}
	else q2outputBug("Can't finish an undefined token.");
}
/**
 * @brief unfinishes the M token pointed to by \p token by setting its number of significant characters to 0
 * 
 * @param token the pointer to the M token
 */void unfinishToken(Mtoken * const token){
	if(token!=NULL)token->significantCharacterCount=0;
	else q2outputBug("Can't finish an undefined token.");
}

uint8_t compareTokens(Mtoken const * const token1,Mtoken const * const token2){
	assert(token1!=NULL&&token2!=NULL);
	uint16_t result=0;
	if(token1->type!=token2->type)result|=1;
	if(token1->significantCharacterCount!=token2->significantCharacterCount)result|=2;
	if(token1->offset!=token2->offset)result|=4;
	if(token1->position!=token2->position)result|=8;
	if(!string_equal(token1->text,token2->text))result|=16;
	if(token1->argument!=token2->argument)result|=32;
	if(token1->envid!=token2->envid)result|=64;
	if(token1->element!=token2->element)result|=128;
	return result;
}
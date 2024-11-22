#include "Mexpression.h"

#include <limits.h>

// MDH@05DEC2020: replaced by Mtoken.h/c in v0.1.5
extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TOKEN,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_BUG_PREFIX;

// MDH@11JUN2020: always call free_token on disowned tokens
Mtoken* owned_token(Mtoken* _token,Mallocationowner owner_token){
	if(NULL==_token)return NULL;
	owned_token(_token->next,owner_token);
	owned_string(_token->text,Msubowner(owner_token,1));
	return OWNED(_token,owner_token);
}
Mtoken* disowned_token(Mtoken* _token,Mallocationowner owner_token){
	if(NULL==_token)return NULL;
	disowned_token(_token->next,owner_token);
	disowned_string(_token->text,Msubowner(owner_token,1));
	return DISOWNED(_token,owner_token);
}
void free_token(Mtoken* _token/*,Mallocationowner owner*/){
	// MDH@19MAY2020: we can free it only when we own it
	if(NULL==_token)return;
	if(_token->next!=NULL){free_token(_token->next/*,owner*/);_token->next=NULL;}
	if(_token->text!=NULL){
		if(amVerboseDebugging())output("Freeing token '%s'.\n",string(_token->text));
		free_string(_token->text/*,owner*/);_token->text=NULL;
	}
	FREE_1(_token,'O'/*,owner*/);
}

Mtoken* __token(){Mallocationowner owner=getOwner(__LINE__);
	return disowned_token(CALLOC_1(sizeof(Mtoken),'O',owner),owner);
}

// MDH@23JUN2020: we might want to change the way we store the number of significant token characters in the future
//				will return a negative value if _token is undefined
size_t getTokenSignificantCharacterCount(Mtoken const * const token){
	if(token!=NULL)return token->significantCharacterCount;
	outputBug("Can't return the number of significant characters of an undefined token.");
	return SIZE_T_MAX; // which is the best value to return to indicate invalid input
}
bool setTokenSignificantCharacterCount(Mtoken * const token,size_t significantCharacterCount){
	if(token!=NULL){
		token->significantCharacterCount=significantCharacterCount;
		return(token->significantCharacterCount==significantCharacterCount);
	}
	outputBug("Can't set the number of significant characters of an undefined token.");
	return false;
}

// a lot of times we're doing the following with the significant character counts
char* _getSignificantTokenCharacters(Mtoken const * const token){
	return(token!=NULL?_stringstart(token->text,token->significantCharacterCount):NULL);
}
Mstring* _getSignificantTokenText(Mtoken const * const token){
	return(token!=NULL?_stringCopy(token->text,token->significantCharacterCount):NULL);
}
Mstring* _getTokenText(Mtoken const * const token){
	return(token!=NULL?_stringCopy(token->text,0):NULL);
}
bool isTokenUnfinished(Mtoken const * const token){
	if(token!=NULL)return(token->significantCharacterCount==0);
	outputBug("Can't determine whether an undefined token is unfinished.");
	return false;
}
bool isTokenFinished(Mtoken const * const token){
	if(token!=NULL)return(token->significantCharacterCount>0);
	outputBug("Can't determine whether an undefined token is finished.");
	return false;
}
void finishToken(Mtoken * const token){
	if(token!=NULL){
		token->significantCharacterCount=string_length(token->text);
		logToOutputFile("Token '%s' finished!",string(token->text));
	}
	else outputBug("Can't finish an undefined token.");
}
void unfinishToken(Mtoken * const token){
	if(token!=NULL)token->significantCharacterCount=0;
	else outputBug("Can't finish an undefined token.");
}
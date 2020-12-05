#include "Mtoken.h"

#include <limits.h>

extern unsigned long long M_MODULE_DEBUGGING;
#define DEBUGGING (M_MODULE_DEBUGGING&MM_TOKEN)

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_TOKEN,id};}

extern char const * const M_ERROR_PREFIX;
extern char const * const M_BUG_PREFIX;

// MDH@11JUN2020: always call free_token on disowned tokens
Mtoken* owned_token(Mtoken* _token,Mallocationowner owner_token){
    if(!_token)return NULL;
    owned_token(_token->next,owner_token);
    owned_string(_token->text,Msubowner(owner_token,1));
    return OWNED(_token,owner_token);
}
Mtoken* disowned_token(Mtoken* _token,Mallocationowner owner_token){
    if(!_token)return NULL;
    disowned_token(_token->next,owner_token);
    disowned_string(_token->text,Msubowner(owner_token,1));
    return DISOWNED(_token,owner_token);
}
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

Mtoken* __token(){Mallocationowner owner=getOwner(__LINE__);
    return disowned_token(CALLOC_1(sizeof(Mtoken),'O',owner),owner);
}

// MDH@23JUN2020: we might want to change the way we store the number of significant token characters in the future
//                will return a negative value if _token is undefined
size_t getTokenSignificantCharacterCount(Mtoken const * const token){
    if(token)return token->significantCharacterCount;
    output("%sCan't return the number of significant characters of an undefined token.\n",M_BUG_PREFIX);
    return SIZE_T_MAX; // which is the best value to return to indicate invalid input
}
bool setTokenSignificantCharacterCount(Mtoken * const token,size_t significantCharacterCount){
    if(token){
        token->significantCharacterCount=significantCharacterCount;
        return(token->significantCharacterCount==significantCharacterCount);
    }
    output("%sCan't set the number of significant characters of an undefined token.\n",M_BUG_PREFIX);
    return false;
}

// a lot of times we're doing the following with the significant character counts
char* _getSignificantTokenCharacters(Mtoken const * const token){
    return(token?_stringstart(token->text,token->significantCharacterCount):NULL);
}
Mstring* _getSignificantTokenText(Mtoken const * const token){
    return(token?_stringCopy(token->text,token->significantCharacterCount):NULL);
}
Mstring* _getTokenText(Mtoken const * const token){
    return(token?_stringCopy(token->text,0):NULL);
}
bool isTokenUnfinished(Mtoken const * const token){
    if(token)return(token->significantCharacterCount==0);
    output("%sCan't determine whether an undefined token is unfinished.",M_BUG_PREFIX);
    return false;
}
bool isTokenFinished(Mtoken const * const token){
    if(token)return(token->significantCharacterCount>0);
    output("%sCan't determine whether an undefined token is finished.",M_BUG_PREFIX);
    return false;
}
void finishToken(Mtoken * const token){
    if(token)token->significantCharacterCount=string_length(token->text);
    else output("%sCan't finish an undefined token.",M_BUG_PREFIX);
}
void unfinishToken(Mtoken * const token){
    if(token)token->significantCharacterCount=0;
    else output("%sCan't finish an undefined token.",M_BUG_PREFIX);
}
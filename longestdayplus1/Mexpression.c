#include "Mexpression.h"

static uint16_t const MODULE_ID=7;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}

extern char const * const M_ERROR_PREFIX;


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
    disowned_string(_token->text,owner_token);
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
    Mtoken* _token=CALLOC_1(sizeof(Mtoken),'O',owner);
    _token->whitespaceCharacterCount=-1; // MDH@22JUN2020: or any other negative value to indicate an unfinished token
    return disowned_token(_token,owner);
}

// MDH@22JUN2020: some special function to take care of (un)finishing tokens or checking whether they are (un)finished
static bool canTokenFinish(Mtoken* _token){return(_token&&_token->whitespaceCharacterCount<0);}
static bool canTokenUnfinish(Mtoken* _token){return(_token&&_token->whitespaceCharacterCount==0);}
bool finishToken(Mtoken* _token){if(canTokenFinish(_token)){_token->whitespaceCharacterCount=0;return true;}return false;}
bool unfinishToken(Mtoken* _token){if(canTokenUnfinish(_token)){_token->whitespaceCharacterCount=-1;return true;}return false;}
bool isTokenFinished(Mtoken* _token){return(_token?_token->whitespaceCharacterCount>=0:false);}
bool isTokenUnfinished(Mtoken* _token){return(_token?_token->whitespaceCharacterCount<0:false);}
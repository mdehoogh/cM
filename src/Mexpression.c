#include "Mexpression.h"

static uint16_t const MODULE_ID=7;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MODULE_ID,id};}

extern char const * const M_ERROR_PREFIX;

Mtoken* __token(){Mallocationowner owner=getOwner(__LINE__);
    return DISOWNED(CALLOC_1(sizeof(Mtoken),'O',owner),owner);
}
void free_token(Mtoken* _token,Mallocationowner owner){
    // MDH@19MAY2020: we can free it only when we own it
    if(!_token)return;
    if(_token->next){free_token(_token->next,owner);_token->next=NULL;}
    if(_token->text){
        if(amVerboseDebugging())output("Freeing token '%s'.\n",string(_token->text));
        free_string(_token->text,owner);_token->text=NULL;
    }
    FREE_DISOWNED_1(_token,'O',owner);
}
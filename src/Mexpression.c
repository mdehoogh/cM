#include "Mexpression.h"

static uint32_t const MODULE_ID=7;
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){(MODULE_ID<<16)+id,0,0};}

extern char const * const M_ERROR_PREFIX;

Mtoken* __token(){Mallocationowner owner=getOwner(__LINE__);
    Mtoken* _token=(Mtoken*)CALLOC(sizeof(Mtoken),'O',owner);
    return DISOWNED(_token,owner);
}

Mtoken* free_token(Mtoken* _token,Mallocationowner owner){
    // MDH@19MAY2020: we can free it only when we own it
    if(OWNED(_token,owner)){
        if(amVerboseDebugging())output("Freeing token '%s'.\n",string(_token->text));
        if(_token->next)free_token(_token->next,owner);
        if(_token->text)free_string(_token->text,owner);
        FREE(_token,'O',owner);
    }
    if(_token)output("Unable to free an unowned token.",M_ERROR_PREFIX);
    return _token;
}
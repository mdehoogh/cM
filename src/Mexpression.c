#include "Mexpression.h"

static int32_t const MODULE_ID=(7<<4);
static int32_t getOwnerId(uint16_t id){return(id>>12?0:(MODULE_ID<<12)+id);}

extern char const * const M_ERROR_PREFIX;

Mtoken* __token(){return (Mtoken*)CALLOC(sizeof(Mtoken),'O',-getOwnerId(1));}

Mtoken* free_token(Mtoken* _token){
    // MDH@19MAY2020: we can free it only when we own it
    if(OWNED(_token,sizeof(Mtoken),getOwnerId(2))){
        if(amVerboseDebugging())output("Freeing token '%s'.\n",string(_token->text));
        if(_token->next)free_token(_token->next);
        if(_token->text)free_string(_token->text);
        FREE(_token,'O',getOwnerId(2));
    }
    if(_token)output("Unable to free an unowned token.",M_ERROR_PREFIX);
    return _token;
}
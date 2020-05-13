#include "Mexpression.h"

void free_token(Mtoken* _token){
    if(_token){
        if(amVerbose()&&amDebugging())output("Freeing token '%s'.\n",string(_token->text));
        if(_token->next)free_token(_token->next);
        if(_token->text)free_string(_token->text);
        FREE(_token,'O');
    }
}
Mtoken* __token(){return (Mtoken*)CALLOC(sizeof(Mtoken),'O');}
/*
void free_expression(Mexpression* _expression){
    if(_expression){
        free_expression(_expression->next);
        free_value(_expression->_value);
        free(_expression);
    }
}
*/
#include "Mexpression.h"

void free_token(Token* _token){
    if(_token){
        free_token(_token->next);
        free_mstring(_token->text);
        free(_token);
    }
}

void free_expression(Mexpression* _expression){
    if(_expression){
        free_token(_expression->first);
        free(_expression);
    }
}
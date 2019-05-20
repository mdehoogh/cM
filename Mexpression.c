#include "Mexpression.h"

void free_token(Mtoken* _token){
    if(_token){
        free_token(_token->next);
        free_mstring(_token->text);
        free(_token);
    }
}
/*
void free_expression(Mexpression* _expression){
    if(_expression){
        free_expression(_expression->next);
        free_value(_expression->_value);
        free(_expression);
    }
}
*/
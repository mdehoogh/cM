#include "Malloc.h"
#include "Mexpression.h"

void free_token(Mtoken* _token){
    if(_token){
        free_token(_token->next);
        free_string(_token->text);
        FREE(_token,'O');
    }
}
Mtoken* __token(){return (Mtoken*)CALLOC(1,sizeof(Mtoken),'O');}
/*
void free_expression(Mexpression* _expression){
    if(_expression){
        free_expression(_expression->next);
        free_value(_expression->_value);
        free(_expression);
    }
}
*/
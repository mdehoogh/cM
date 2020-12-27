#include "Mjson.h"

extern unsigned long long M_MODULE_DEBUGGING;
#define DEBUGGING (M_MODULE_DEBUGGING&MM_JSON)

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_JSON,id};}

Mstring* json_addproperty(Mstring * const str,char const * const propertyName,bool first){
    Mstring* p=str;
    if(p){
        if(!first)p=string_append(p,JSON_ITEM_SEPARATOR);
        if(propertyName){
		    p=string_append(p,JSON_PROPERTY_NAME_START);
		    p=string_append(p,propertyName);
		    p=string_append(p,JSON_PROPERTY_NAME_END);
		    p=string_append(p,JSON_PROPERTY_NAME_VALUE_SEPARATOR);
        }
    }
    return p;
}
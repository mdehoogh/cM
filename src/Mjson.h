#include "Mstring.h"

// MDH@26DEC2020
#define JSON_OBJECT_START "{"
#define JSON_OBJECT_END "}"
#define JSON_ARRAY_START "["
#define JSON_ARRAY_END "]"
#define JSON_ITEM_SEPARATOR ","
#define JSON_PROPERTY_NAME_START "\""
#define JSON_PROPERTY_NAME_END "\""
#define JSON_PROPERTY_NAME_VALUE_SEPARATOR ":"

Mstring* json_addproperty(Mstring * const str,char const * const propertyName,bool first);
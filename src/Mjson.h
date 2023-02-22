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

// MDH@29DEC2020: how about parsing text to and from an Mjson structure????
// a JSON object is simply a property bag where each property value is either a simple value (stored as Mstring) or a composite value (either Mjsonobject or Mjsonobject)
/**
 * @brief Mjsonvaluetype defines the different types of JSON structures JVT_OBJECT, JVT_ARRAY and JVT_VALUE
 * 
 */
typedef enum Mjsonvaluetype {JVT_OBJECT,JVT_ARRAY,JVT_VALUE}Mjsonvaluetype;
struct Mjsonobject;
struct Mjsonarray;

/**
 * @brief a Mjsonvalueunion defines either an Mjsonobject or a Mjsonarray
 * 
 */
typedef union Mjsonvalueunion{
    char* _chars;
    struct Mjsonobject* _object;
    struct Mjsonarray* array;
}Mjsonvalueunion;

/**
 * @brief a Mjsonvalue represents a single possible JSON value as either a JSON value, JSON array or JSON object
 * 
 */
typedef struct Mjsonvalue{
    Mjsonvaluetype valuetype;
    Mjsonvalueunion* _value;
}Mjsonvalue;

/**
 * @brief a Mjsonproperty represents a property in an JSON object
 * 
 */
typedef struct Mjsonproperty{
    char* _name;
    Mjsonvalue* _value;
    struct Mjsonproperty* _next;
}Mjsonproperty;

/**
 * @brief a Mjsonobject is a linked list of Mjson properties
 * 
 */
typedef struct Mjsonobject{
    Mjsonproperty* _firstproperty;
}Mjsonobject;

/**
 * @brief a Mjsonarray is a sequence of JSON objects
 * 
 */
typedef struct Mjsonarray{
    size_t numberOfValues;
    Mjsonobject* _values[1];
}Mjsonarray;

Mstring* json_addproperty(Mstring * const str,char const * const propertyName,bool first);

Mjsonvalue* json_getpropertyvalue(Mjsonobject const * const _jsonobject,char const * const propertyName);

Mjsonvalue* json_parse(char const * const _text,size_t *pos);
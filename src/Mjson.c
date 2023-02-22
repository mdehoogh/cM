#include "Mjson.h"

extern unsigned long long M_MODULE_DEBUGGING;
extern const char * const M_ERROR_PREFIX;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_JSON,id};}

/**
 * @brief disownes the JSON value pointed to by \p _jsonvalue currently owned by \p owner_jsonvalue
 * 
 * @param _jsonvalue the pointer to the JSON value to be disowned
 * @param owner_jsonvalue the current owner of the JSON value
 * @return Mjsonvalue* the pointer to the disowned JSON value pointed to by \p _jsonvalue
 */
Mjsonvalue* disowned_jsonvalue(Mjsonvalue* _jsonvalue,Mallocationowner owner_jsonvalue){
	if(!_jsonvalue)return NULL;
	switch(_jsonvalue->valuetype){
		case JVT_ARRAY:
			{

			}
			break;
		case JVT_OBJECT:
			{

			}
			break;
		case JVT_VALUE:
			DISOWNED(_jsonvalue->_value->_chars,owner_jsonvalue);
	}
	return DISOWNED(_jsonvalue,owner_jsonvalue);
}

/**
 * @brief creates and returns a pointer to a new JSON value
 * 
 * @param source identifies the caller
 * @return Mjsonvalue* a pointer to a disowned JSON value
 */
Mjsonvalue* __jsonvalue(char* source){Mallocationowner owner=getOwner(__LINE__);
	Mjsonvalue* _jsonvalue=CALLOC_1(sizeof(Mjsonobject),'J',owner);
	return disowned_jsonvalue(_jsonvalue,owner);
}

/**
 * @brief assigns the ownership of the JSON value pointed to by \p _jsonvalue to \p owner_jsonvalue
 * 
 * @param _jsonvalue the pointer to the JSON value
 * @param owner_jsonvalue the new owner of the JSON value
 * @return Mjsonvalue* the newly owned pointer \p _jsonvalue
 */
Mjsonvalue* owned_jsonvalue(Mjsonvalue* _jsonvalue,Mallocationowner owner_jsonvalue){
	// TODO obtain ownership of fields
	return OWNED(_jsonvalue,owner_jsonvalue);
}

/**
 * @brief frees the JSON value pointed to by \p _jsonvalue
 * 
 * @param _jsonvalue the pointer to the JSON value to be freed
 */
void free_jsonvalue(Mjsonvalue* _jsonvalue){
	
	FREE_1(_jsonvalue,'J');
}

/**
 * @brief add the property with name \p propertyName to the characters stored in \p str
 * 
 * 
 * @param str the pointer to the characters to append \p propertyName to
 * @param propertyName the name of the property to append
 * @param first the flag indicating whether or not this is the first JSON object property
 * @return Mstring* \p str with the property name appended in front of a JSON property value as part of the text representation of a JSON object
 */
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

// MDH@29DEC2020: retrieve a property by name from a JSON encoded str
/**
 * @brief returns the JSON value in the JSON object pointed to by \p _jsonobject of property \p propertyName
 * 
 * @param _jsonobject the pointer to the JSON object
 * @param propertyName the name of the property
 * @return Mjsonvalue* the JSON value of property \p propertyName in the JSON object pointed to by \p _jsonobject
 */
Mjsonvalue* json_getpropertyvalue(Mjsonobject const * const _jsonobject,char const * const propertyName){
	Mjsonvalue* jsonvalue=NULL;
	if(_jsonobject&&propertyName){
		Mjsonproperty* jsonproperty=_jsonobject->_firstproperty;
		while(jsonproperty&&strcmp(jsonproperty->_name,propertyName))jsonproperty=jsonproperty->_next;
		if(jsonproperty)jsonvalue=jsonproperty->_value;
	}
	return jsonvalue;
}

// any text can be parsed into a value (if possible), starting at a given position (which will end on the last character of the value)
/**
 * @brief parses the JSON text representation of a JSON value pointed to by \p _text starting at position \p pos
 * 
 * @param _text the text reprentation of a JSON value starting at position \p pos
 * @param pos the first position of the text representation of a JSON value
 * @return Mjsonvalue* the parsed pointer to the JSON value
 */
Mjsonvalue* json_parse(char const * const _text,size_t *pos){Mallocationowner owner=getOwner(__LINE__);
	if(_text){
		size_t l=strlen(_text);
		if(*pos<l){ // there's text to parse behind pos
			// should we skip all whitespace first????
			Mjsonvalue* _jsonvalue=owned_jsonvalue(__jsonvalue("_parsedJSONvalue"),owner);
			if(_jsonvalue){

			}else
				output("%sFailed to parse JSON value text '%s'.\n",M_ERROR_PREFIX,_text);
			return disowned_jsonvalue(_jsonvalue,owner);
		}
	}
	return NULL;
}
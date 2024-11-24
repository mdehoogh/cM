#include "Marray.h"

extern unsigned long long M_MODULE_DEBUGGING;

// MDH@22NOV2020
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_ARRAY,id};}

extern long long M_LL_INVALID,M_TRUE,M_FALSE;

// iterator support
/**
 * @brief returns the next sequence element of an array M iterator \p iterator
 * @details 
 * @param iterator 
 * @return Msequenceelement the next sequence element of M array iterator \p iterator
 */
static Msequenceelement arrayNext(void * const iterator){
	Msequenceelement sequenceelement={};
	if(iterator!=NULL){
		Miterator* it=(Miterator*)iterator;
		if(it->valueholder!=NULL){
			sequenceelement=(Msequenceelement){it->index,*(it->valueholder)};
			if(it->index<it->lastindex){ // not the last array element yet
				// we need the last value holder to know when we are done!!!!
				it->index++;
				it->valueholder++;
			}else // end of array reached
				it->valueholder=NULL;
		}
	}
	return sequenceelement;
}
/**
 * @brief returns the next index of M array iterator \p iterator
 * 
 * @param iterator 
 * @return unsigned long long the next index of M array iterator
 */
static unsigned long long arrayNextIndex(void * const iterator){
	if(iterator!=NULL){
		Miterator* it=(Miterator*)iterator;
		if(it->valueholder!=NULL)return it->index;
	}
	return 0;
}
/**
 * @brief returns an M array iterator into source M array \p array
 * 
 * @param array 
 * @return Miterator an M array iterator into M array \p array
 */
Miterator getArrayiterator(Marray* array){
	Miterator arrayiterator={arrayNext,arrayNextIndex,1};
	if(array!=NULL){
		arrayiterator.valuetype=array->valuetype;
		if(array->numberOfElements){
			arrayiterator.lastindex=array->numberOfElements;
			arrayiterator.valueholder=(void**)array->values; // NOTE leading in determining whether end of sequence was reached (instead of index)
		}
	}
	return arrayiterator;
}

/**
 * @brief returns a wrapper containing an M array with \p length_value elements initialized to \p fill_value
 * @details if \p length_value is a map, the result array is initialized with the name value pairs of the input map
 * @details if \p length_value is an array, the result array is initialized with the elements of the input array
 * @param length_value 
 * @param fill_value 
 * @return Mvalue* a wrapper containing an M array constructed from the input
 */
Mvalue* Mcreatearray(Mvalue* length_value,Mvalue* fill_value){Mallocationowner owner=getOwner(__LINE__);
	// how about allowing length_value to be a list or array to fill the array with 
	// returns an array that can store length_value values (if possible)
	// MDH@09APR2023: now also allowing length_value to contain an array of dimension thus indicating that this will be a multi-dimensional array (instead of a flat array)
	if(length_value!=NULL){
		long long length;
		if(length_value->type==VT_MAP){
			Mmap* _map=length_value->value._map;
			length=_map->numberOfElements<<1;
			Marray* _result=owned_array(_getArray("marray",length,NULL),owner); // each key value pair uses 2 array elements
			if(_result!=NULL){
				Mvariable* variable;
				Mmapelement* mapelement=_map->_first;
				unsigned long long arrayindex=0;
				while(mapelement!=NULL&&arrayindex<length){
					// NOTE the name of the variable is an Mchars which essentially means it does not know its quote character so in essence we need to wrap it first
					Mstring* _name=owned_string(_getString("'"),owner);
					if(_name!=NULL){
						if(string_append(_name,mapelement->_variable->_name->chars)){
							assignValue(&_result->values[arrayindex++],_getTextValue(string(_name)));
							assignValue(&_result->values[arrayindex++],mapelement->_variable->_value);
						}
						FREE_STRING(_name,owner);
					}
					mapelement=mapelement->_next;
				}
				return _getValueOfArray(disowned_array(_result,owner));
			}
		}else
		if(length_value->type==VT_ARRAY){
			// MDH@09APR2023: we should actually use the positive integers in the array to initialize the array
			Marray* _array=length_value->value._array;
			unsigned long long numberOfDimensions=_array->numberOfElements;
			// creating the dimensions backwards
			Marray*_result=NULL;
			Mvalue* fillValue=fill_value; // the fill value of the array at the given dimension
			while(numberOfDimensions>0){
				long long arrayLength=getValueInteger(_array->values[--numberOfDimensions]);
				if(arrayLength>0){
					if(amVerbose()){
						q2output("Filling %lld elements of array",arrayLength);
						q2outputValue(" with ",fillValue,".\n");
					}
					_result=owned_array(_getArray("marray",arrayLength,fillValue),owner);
					if(_array!=NULL){
						fillValue=_getValueOfArray(disowned_array(_result,owner));
					}else
						q2outputError("Failed to create (sub)array");
				}else
					q2outputError("Invalid (sub)array length");
			}
			return fillValue;
			/* replacing:
			length=_array->numberOfElements;
			Marray* _result=owned_array(_getArray("marray",length),owner);
			if(_result!=NULL){
				while(--length>=0)assignValue(&_result->values[length],_array->values[length]);
				return _getValueOfArray(disowned_array(_result,owner));
			}
			*/
		}else
		if(length_value->type==VT_LIST){
			// a list is problematic since we need the dimensions in reverse order
			Mlist* _reversedList=owned_list(_getReversedList(length_value->value._list),owner);
			if(_reversedList!=NULL){
				Mlistelement* listelement=_reversedList->_first;
				unsigned long long numberOfDimensions=_reversedList->numberOfElements;
				Mvalue* fillValue=fill_value;
				Marray* _result=NULL;
				while(listelement!=NULL&&numberOfDimensions>0){
					long long arrayLength=getValueInteger(listelement->_value);
					if(arrayLength>0){
						_result=owned_array(_getArray("marray",arrayLength,fillValue),owner);
						if(_result!=NULL)
							fillValue=_getValueOfArray(disowned_array(_result,owner));
						else
							q2outputError("Failed to create (sub)array");
					}else
						q2outputError("Invalid (sub)array length");
					listelement=listelement->_next;
				}
				FREE_LIST(_reversedList,owner);
				return fillValue;
			}
			/* replacing:
			Mlist* _list=length_value->value._list;
			length=_list->numberOfElements;
			Marray* _result=owned_array(_getArray("marray",length,NULL),owner);
			if(_result!=NULL){
				Mlistelement* listelement=_list->_first;
				unsigned long long arrayindex=0;
				while(listelement!=NULL&&arrayindex<length){assignValue(&_result->values[arrayindex++],listelement->_value);listelement=listelement->_next;}
				return _getValueOfArray(disowned_array(_result,owner));
			}
			*/
		}else{ // not composite, interpret as a length
			long long length=getValueInteger(length_value);
			if(length>=0){
				Marray* _result=owned_array(_getArray("marray",length,fill_value),owner);
				if(_result!=NULL)return _getValueOfArray(disowned_array(_result,owner));
				q2outputError("Failed to create array");
			}
		}
	}
	return NULL;
}

/**
 * @brief fills the M array wrapped in \p array_value with \p value
 * 
 * @param array_value 
 * @param value 
 * @return Mvalue* \p array_value with all elements set to \p value
 */
Mvalue* Mfillarray(Mvalue* array_value,Mvalue* value){
	Marray* array=(array_value&&array_value->type==VT_ARRAY?array_value->value._array:NULL);
	if(array!=NULL){
		unsigned long long l=array->numberOfElements;
		// we use assignValue here we get a copy of maps and lists (and arrays for that matter)
		while(l>0)assignValue(&array->values[--l],value);
	}
	return array_value;
}

#include "Moperations.h"

extern long long M_LL_INVALID;
extern long double M_LD_NAN;
extern long long M_TRUE;
extern long long M_FALSE;
extern Mdecimalcontext* M_DECIMALCONTEXT;

static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_OPERATIONS,id};}

// BINARY OPERATOR + functions

// helper functions that apply binary operators to value of which at least one is a list
// MDH@03NOV2020: I suppose we can have a method that will provide us with the value type for applying binary operators that maintain type
/**
 * @brief returns \p listValuetype when \p listValuetype equals \p valueValuetype or VT_UNDEFINED otherwise
 * 
 * @param listValuetype 
 * @param valueValuetype 
 * @return Mvaluetype \p listValuetype when \p listValuetype equals \p valueValuetype or VT_UNDEFINED otherwise
 */
static Mvaluetype getMatchingListValuetype(Mvaluetype listValuetype,Mvaluetype valueValuetype){
	// essentially the matching list value type is listValuetype unless valueValuetype is different
	// TODO we might improve on this if we select the 'highest' of the two value types as the result type
	return(listValuetype!=valueValuetype?VT_UNDEFINED:listValuetype);
}
// we can use a single function to apply a certain binary operator because the functions have the same signature as a TwoArgumentFunction!!
/**
 * @brief returns \p arrayValuetype when \p arrayValuetype equals \p valueValuetype or VT_UNDEFINED otherwise
 * 
 * @param arrayValuetype 
 * @param valueValuetype 
 * @return Mvaluetype \p arrayValuetype when \p arrayValuetype equals \p valueValuetype or VT_UNDEFINED otherwise
 */
static Mvaluetype getMatchingArrayValuetype(Mvaluetype arrayValuetype,Mvaluetype valueValuetype){
	// essentially the matching list value type is listValuetype unless valueValuetype is different
	// TODO we might improve on this if we select the 'highest' of the two value types as the result type
	return(arrayValuetype!=valueValuetype?VT_UNDEFINED:arrayValuetype);
}

/**
 * @brief returns the list containing the result of applying \p oneArgumentFunction to each corresponding element in \p _list
 * 
 * @param _list 
 * @param oneArgumentFunction 
 * @param maintainsValuetype the list containing the result of applying \p oneArgumentFunction to each corresponding element in \p _list
 * @return Mlist* 
 */
/*
Mlist* _appliedToListElements(Mlist* _list,OneArgumentFunction oneArgumentFunction,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _result=(NULL==_list?NULL:owned_list(_getListOfType(maintainsValuetype?_list->valuetype:VT_UNDEFINED),owner)); // TODO if the types are the same use that?
	if(_result!=NULL){
		// elements with the same index are to be added and stored under that index
		Mlistelement* _listelement=_list->_first;
		while(_listelement){
			if(appendedToList(_result,owner,oneArgumentFunction(_listelement->_value),_listelement->index)<=0)break;
			_listelement=_listelement->_next;
		}
		return disowned_list(_result,owner);
	}
	return NULL;
}
*/
/**
 * @brief applies binary operator \p binaryoperator to the elements in \p list1 and \p list2
 * 
 * @param _list1 
 * @param _list2 
 * @param maintainsValuetype 
 * @return Mlist* the wrapped M list of the result of applying \p binaryoperator to the elements of \p list1 and \p list2
 */
Mlist* _appliedToLists(Mlist* _list1,Mlist* _list2,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_list1)return _list2;if(NULL==_list2)return _list1;
	// MDH@30OCT2019: ALWAYS apply the binary operator i.e. do NOT just return the value!!! (which makes perfect sense for equality / unequality)
	//				TODO if the result equals NULL, should we then NOT add the given element?????
	// ASSERT neither are NULL
	// MDH@03NOV2020: the return type of the list really depends on the binary operator applied, whether or not it maintains type integrity, so it makes sense to actually pass in the list result type as a separate argument
	Mlist* _result=owned_list(_getListOfType(maintainsValuetype?getMatchingListValuetype(_list1->valuetype,_list2->valuetype):VT_UNDEFINED),owner); // TODO if the types are the same use that?
	if(_result!=NULL){
		// elements with the same index are to be added and stored under that index
		Mlistelement* _listelement1=_list1->_first;
		Mlistelement* _listelement2=_list2->_first;
		while(_listelement1!=NULL||_listelement2!=NULL){
			if(_listelement1!=NULL&&_listelement2!=NULL){
				if(_listelement1->index==_listelement2->index){
					if(appendedToList(_result,owner,binaryoperator(_listelement1->_value,_listelement2->_value),_listelement1->index)<=0)break;
					_listelement1=_listelement1->_next;_listelement2=_listelement2->_next;
				}else
				if(_listelement1->index<_listelement2->index){
					if(appendedToList(_result,owner,binaryoperator(_listelement1->_value,NULL),_listelement1->index)<=0)break;
					_listelement1=_listelement1->_next;
				}else{
					if(appendedToList(_result,owner,binaryoperator(NULL,_listelement2->_value),_listelement2->index)<=0)break;
					_listelement2=_listelement2->_next;
				}
			}else
			if(_listelement1!=NULL){
				if(appendedToList(_result,owner,binaryoperator(_listelement1->_value,NULL),_listelement1->index)<=0)break;
				_listelement1=_listelement1->_next;
			}else{
				if(appendedToList(_result,owner,binaryoperator(NULL,_listelement2->_value),_listelement2->index)<=0)break;
				_listelement2=_listelement2->_next;
			}
		}
	}
	return disowned_list(_result,owner);
}
/**
 * @brief applies binary operator \p binaryoperator to the elements of M list \p _list and M array \p _array
 * 
 * @param _list 
 * @param _array 
 * @param maintainsValuetype 
 * @return Mlist* returns the wrapped M list containing the result of applying \p binaryoperator to M list \p _list and M array \p _array
 */
Mlist* _appliedToListAndArray(Mlist* _list,Marray* _array,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_array||_array->numberOfElements==0)return _list;
	Mlist* _result=owned_list(_getListOfType(maintainsValuetype?getMatchingListValuetype(_list->valuetype,_array->valuetype):VT_UNDEFINED),owner); // TODO if the types are the same use that?
	if(_result!=NULL){
		// elements with the same index are to be added and stored under that index
		Mlistelement* _listelement=_list->_first;
		unsigned long long arrayindex=0;
		while(_listelement!=NULL||arrayindex<_array->numberOfElements){
			if(_listelement!=NULL&&arrayindex<_array->numberOfElements){
				if(_listelement->index==arrayindex+1){
					if(appendedToList(_result,owner,binaryoperator(_listelement->_value,_array->values[arrayindex]),arrayindex+1)<=0)break;
					_listelement=_listelement->_next;arrayindex++;
				}else
				if(_listelement->index<=arrayindex){
					if(appendedToList(_result,owner,binaryoperator(_listelement->_value,NULL),_listelement->index)<=0)break;
					_listelement=_listelement->_next;
				}else{
					if(appendedToList(_result,owner,binaryoperator(NULL,_array->values[arrayindex]),arrayindex+1)<=0)break;
					arrayindex++;
				}
			}else
			if(_listelement!=NULL){
				if(appendedToList(_result,owner,binaryoperator(_listelement->_value,NULL),_listelement->index)<=0)break;
				_listelement=_listelement->_next;
			}else{
				if(appendedToList(_result,owner,binaryoperator(NULL,_array->values[arrayindex]),arrayindex+1)<=0)break;
				arrayindex++;
			}
		}
	}
	return disowned_list(_result,owner);
}
/**
 * @brief applies binary operator \p binaryoperator to array \p _array and M list \p _list
 * 
 * @param _array 
 * @param _list 
 * @param binaryoperator
 * @param maintainsValuetype 
 * @return Marray* the wrapped M array containing the result of applying \p binaryoperator to the elements of M array \p _array and M list \p _list
 */
Marray* _appliedToArrayAndList(Marray* _array,Mlist* _list,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_list||_list->numberOfElements==0)return _array;
	// ASSERT the list is not empty
	unsigned long long arraylength=(_array!=NULL?_array->numberOfElements:0);
	// the number of elements in the array is the maximum of the number of elements in the array or the index of the list
	Marray* _result=owned_array(_getArray("appliedToArrayAndList",MAX(arraylength,_list->_last->index),NULL),owner);
	if(_result!=NULL){
		if(maintainsValuetype)_result->valuetype=_array->valuetype; // MDH@29MAR2023: TODO should we do this????
		// elements with the same index are to be added and stored under that index
		Mlistelement* _listelement=_list->_first;
		unsigned long long arrayindex=0;
		while(_listelement!=NULL||arrayindex<arraylength){
			if(_listelement!=NULL&&arrayindex<arraylength){
				if(_listelement->index==arrayindex+1){
					assignValue(&_result->values[arrayindex],binaryoperator(_array->values[arrayindex],_listelement->_value));
					_listelement=_listelement->_next;arrayindex++;
				}else
				if(_listelement->index<=arrayindex){
					assignValue(&_result->values[_listelement->index-1],binaryoperator(NULL,_listelement->_value));
					_listelement=_listelement->_next;
				}else{
					assignValue(&_result->values[arrayindex],binaryoperator(_array->values[arrayindex],NULL));
					arrayindex++;
				}
			}else
			if(_listelement!=NULL){
				assignValue(&_result->values[_listelement->index-1],binaryoperator(NULL,_listelement->_value));
				_listelement=_listelement->_next;
			}else{
				assignValue(&_result->values[arrayindex],binaryoperator(_array->values[arrayindex],NULL));
				arrayindex++;
			}
		}
	}
	return disowned_array(_result,owner);
}

// we can use a single function to apply a certain binary operator because the functions have the same signature as a TwoArgumentFunction!!
/**
 * @brief applies the binary operator \p binaryoperator to each element in M list \p _list with \p _value
 * 
 * @param _list 
 * @param _value 
 * @param binaryoperator
 * @param maintainsValuetype 
 * @return Mvalue* returns the wrapped M list of applying \p binaryoperator to each element in M list \p _list and \p _value
 */
Mvalue* _appliedToList(Mlist* _list,Mvalue* _value,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mvalue* resultValue=NULL;
	if(_value!=NULL){
		if(_value->type!=VT_LIST&&_value->type!=VT_ARRAY){
			// bool resultsOfSameType=(_list->valuetype!=VT_UNDEFINED);
			Mlist* _result=owned_list(_getListOfType(maintainsValuetype?getMatchingListValuetype(_list->valuetype,_value->type):VT_UNDEFINED),owner);
			if(_result!=NULL){
				Mlistelement* _listelement=_list->_first;
				while(_listelement!=NULL){
					Mvalue* resultValue=binaryoperator(_listelement->_value,_value);
					if(appendedToList(_result,owner,resultValue,_listelement->index)<=0){
						outputError("Failed to store the result of applying a binary operator");
						break;
					}
					// if(resultValue)if(resultValue->type!=_list->valuetype)resultsOfSameType=false;
					_listelement=_listelement->_next;
				}
				resultValue=_getValueOfList(disowned_list(_result,owner));
			}
		}else
		if(_value->type==VT_LIST)
			resultValue=_getValueOfList(_appliedToLists(_list,_value->value._list,binaryoperator,maintainsValuetype));
		else
			resultValue=_getValueOfList(_appliedToListAndArray(_list,_value->value._array,binaryoperator,maintainsValuetype));
	}
	return resultValue;
}
/**
 * @brief applies binary operator \p binaryoperator to \p _value and each element in M list \p _list
 * 
 * @param _value 
 * @param _list 
 * @param binaryoperator
 * @param maintainsValuetype 
 * @return Mvalue* the wrapped M list with results of applying \p binaryoperator to \p _value and elements in \p _list
 */
Mvalue* _appliedToList2(Mvalue* _value,Mlist* _list,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mvalue* resultValue=NULL;
	if(_value!=NULL){
		if(_value->type!=VT_LIST&&_value->type!=VT_ARRAY){
			// bool resultsOfSameType=(_list->valuetype!=VT_UNDEFINED);
			Mlist* _result=owned_list(_getListOfType(maintainsValuetype?getMatchingListValuetype(_list->valuetype,_value->type):VT_UNDEFINED),owner);
			if(_result!=NULL){
				Mlistelement* _listelement=_list->_first;
				while(_listelement!=NULL){
					Mvalue* resultValue=binaryoperator(_value,_listelement->_value);
					if(appendedToList(_result,owner,resultValue,_listelement->index)<=0)break;
					// if(resultValue)if(resultValue->type!=_list->valuetype)resultsOfSameType=false;
					_listelement=_listelement->_next;
				}
				resultValue=_getValueOfList(_result);
			}
		}else
		if(_value->type==VT_LIST)
			resultValue=_getValueOfList(_appliedToLists(_value->value._list,_list,binaryoperator,maintainsValuetype));
		else
			resultValue=_getValueOfArray(_appliedToArrayAndList(_value->value._array,_list,binaryoperator,maintainsValuetype));
	}
	return resultValue;
}
/*
Marray* _appliedToArrayElements(Marray* _array,OneArgumentFunction oneArgumentFunction,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	Marray* _result=(NULL==_array?NULL:owned_array(_getArray("_appliedToArrayElements",_array->numberOfElements,NULL),owner)); // TODO if the types are the same use that?
	if(_result!=NULL){
		if(maintainsValuetype)_result->valuetype=_array->valuetype;
		unsigned long long arrayindex=0;
		while(arrayindex<_result->numberOfElements){
			// leaving it up to the binary operator what will be the result of applying it with one of the arguments equal to NULL
			assignValue(&_result->values[arrayindex],oneArgumentFunction(_array->values[arrayindex]));
			arrayindex++;
		}
		return disowned_array(_result,owner);
	}
	return NULL;
}
*/
/**
 * @brief applies \p binaryoperator to the elements in M array \p _array1 and M array \p _array2
 * 
 * @param _array1 
 * @param _array2 
 * @param maintainsValuetype 
 * @return Marray* returns the wrapped M array with results of applying \p binaryoperator to the associated elements in \p _array1 and \p array2
 */
Marray* _appliedToArrays(Marray* _array1,Marray* _array2,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// MDH@30MAR2023: it's debatable whether we want a elementwise binary operator application or every element with every other element
	if(NULL==_array1)return _array2;if(NULL==_array2)return _array1;
	Marray* _result=owned_array(_getArray("_appliedToArrays",MAX(_array1->numberOfElements,_array2->numberOfElements),NULL),owner); // TODO if the types are the same use that?
	if(_result!=NULL){
		if(maintainsValuetype)_result->valuetype=getMatchingArrayValuetype(_array1->valuetype,_array2->valuetype);
		unsigned long long arrayindex=0;
		while(arrayindex<_result->numberOfElements){
			// leaving it up to the binary operator what will be the result of applying it with one of the arguments equal to NULL
			assignValue(&_result->values[arrayindex],binaryoperator((arrayindex<_array1->numberOfElements?_array1->values[arrayindex]:NULL),(arrayindex<_array2->numberOfElements?_array2->values[arrayindex]:NULL)));
			arrayindex++;
		}
	}
	return disowned_array(_result,owner);
}
/**
 * @brief applies \p binaryoperator to each element in M array \p _array and \p _value
 * 
 * @param _array 
 * @param _value 
 * @param binaryoperator
 * @param maintainsValuetype 
 * @return Mvalue* the wrapped M array of applying \p binaryoperator to each element in M array \p _array and \p _value
 */
Mvalue* _appliedToArray(Marray* _array,Mvalue* _value,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mvalue* resultValue=NULL;
	if(_value!=NULL){
		if(_value->type!=VT_ARRAY&&_value->type!=VT_LIST){
			// should create an array of the same length
			Marray* _result=owned_array(_getArray("_appliedToArray",_array->numberOfElements,NULL),owner);
			if(_result!=NULL){
				if(maintainsValuetype)_result->valuetype=getMatchingArrayValuetype(_array->valuetype,_value->type);
				unsigned long long arrayindex=0;
				while(arrayindex<_array->numberOfElements){
					assignValue(&_result->values[arrayindex],binaryoperator(_array->values[arrayindex],_value));
					arrayindex++;
				}
				resultValue=_getValueOfArray(disowned_array(_result,owner));
			}
		}else
		if(_value->type==VT_ARRAY)
			resultValue=_getValueOfArray(_appliedToArrays(_array,_value->value._array,binaryoperator,maintainsValuetype));
		else
			resultValue=_getValueOfArray(_appliedToArrayAndList(_array,_value->value._list,binaryoperator,maintainsValuetype));
	}
	return resultValue;
}
/**
 * @brief applies \p binaryoperator to \p _value and each element of M array \p _array
 * 
 * @param _value 
 * @param _array 
 * @param maintainsValuetype 
 * @return Mvalue* the wrapped 
 */
Mvalue* _appliedToArray2(Mvalue* _value,Marray* _array,TwoArgumentFunction binaryoperator,bool maintainsValuetype){Mallocationowner owner=getOwner(__LINE__);
	// scalars are to be added to each element of the original list
	// lists are to be added to the elements at the same position, so listwise
	Mvalue* resultValue=NULL;
	if(_value!=NULL){
		if(_value->type!=VT_ARRAY&&_value->type!=VT_LIST){
			// should create an array of the same length
			Marray* _result=owned_array(_getArray("_appliedToArray2",_array->numberOfElements,NULL),owner);
			if(_result!=NULL){
				if(maintainsValuetype)_result->valuetype=getMatchingArrayValuetype(_array->valuetype,_value->type);
				unsigned long long arrayindex=0;
				while(arrayindex<_array->numberOfElements){
					assignValue(&_result->values[arrayindex],binaryoperator(_array->values[arrayindex],_value));
					arrayindex++;
				}
			}
		}else
		if(_value->type==VT_ARRAY)
			resultValue=_getValueOfArray(_appliedToArrays(_value->value._array,_array,binaryoperator,maintainsValuetype));
		else
			resultValue=_getValueOfList(_appliedToListAndArray(_value->value._list,_array,binaryoperator,maintainsValuetype));
	}
	return resultValue;
}

/**
 * @brief returns the negated value of \p _value
 * 
 * @param _value 
 * @return Mvalue* the negated value of \p _value
 */
Mvalue* Mneg(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__); // negate a value
	if(_value!=NULL){
		if(_value->type==VT_INTEGER)return _getIntegerValue(-_value->value._integer->ll);
		if(_value->type==VT_FLOAT)return _getFloatValue(-_value->value._float->ld);
		if(_value->type==VT_ARRAY)return _getValueOfArray(appliedToArray(_value->value._array,Mneg,VT_UNDEFINED));
		if(_value->type==VT_LIST)return _getValueOfList(appliedToList(_value->value._list,Mneg,VT_UNDEFINED));
		if(_value->type==VT_MAP)return _getValueOfMap(appliedToMap(_value->value._map,Mneg,VT_UNDEFINED));
		if(_value->type==VT_BIGINTEGER)return _getValueOfBiginteger(_getNegatedBiginteger(_value->value._biginteger));
		if(_value->type==VT_RATIONAL){
			// this is done by negating the numerator but if the numerator equals NULL we should use -1
			Mrational* rational=_value->value._rational;
			if(rational!=NULL){
				/////////q2outputValue("Negating rational '",_value,"'.\n");
				Mbiginteger* _biNumerator=owned_biginteger(rational->num?_getNegatedBiginteger(rational->num):_getBiginteger(-1),owner); // negating the numerator
				if(_biNumerator!=NULL){
					////////q2outputBiginteger("Denominator '",_biDenominator,"' copied!\n");
					Mrational* _negRational=_getRational(_biNumerator,rational->den,(isFloatUndefined(rational->delta)==M_TRUE?M_LD_NAN:-rational->delta->ld),false);
					FREE_BIGINTEGER(_biNumerator,owner);
					return _getValueOfRational(disowned_rational(_negRational,owner));
				}
				outputError("Failed to negate the numerator of a rational");
			}
		}else
		if(_value->type==VT_DECIMAL){
			Mdecimal* decimal=_value->value._decimal;
			if(decimal!=NULL){
				Mdecimal* _negDecimal=owned_decimal(__decimal(M_DECIMALCONTEXT->mpd_context,0,0),owner);
				if(_negDecimal!=NULL){
					uint32_t status=0;
					mpd_qcopy_negate(_negDecimal->mpd,decimal->mpd,&status);
					if(status&0xEFBF){
						FREE_DECIMAL(_negDecimal,owner);_negDecimal=NULL;
						outputError("Failed to negate a decimal");
						outputDecimalStatus(status);
					}else // success, ascertain to copy the repeating field over as that remains the same on negating (assumedly)
						_negDecimal->repeating=decimal->repeating;
					if(_negDecimal!=NULL)return _getValueOfDecimal(disowned_decimal(_negDecimal,owner));
				}
			}
		}
	}
	return NULL;
}/* VALIDATED */

// two-argument arithmetic
// helper functions
// NOTE the following takes a lot of precision because we should never return the originals always copies which should be freed if they are not used anymore
/* see Mexecution.c
Mbiginteger* _getBigintegerCopy(Mbiginteger* _biginteger){
	Mbiginteger* _bigintegerCopy=new_Mbiginteger();if(mp_copy(_biginteger,_bigintegerCopy)!=MP_OKAY){FREE_BIGINTEGER(_bigintegerCopy);return NULL;}return _bigintegerCopy;
}
*/
// rational number addition
// generic addition
///// MDH@18NOV2019 is now defined elsewhere!!: Mdecimal* getValueDecimal(Mvalue* _value);
/**
 * @brief returns the sum of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the sum of \p _value1 and \p _value2
 */
Mvalue* Madd(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL; // MDH@24OCT2019: propagate NULL
	// if either is a list apply 'add' to the list (NOTE scalar addition is NOT the same as list addition)
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Madd,true);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Madd,true);
	if(_value2->type==VT_ARRAY)return _appliedToArray(_value2->value._array,_value1,Madd,true);
	if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,Madd,true);
	// MDH@24OCT2019: isValueZero() can now also return M_LL_INVALID and we do NOT want the value to be considered a 'true' zero when that happens!!!!!
	if(isValueZero(_value1)==M_TRUE)return _value2;
	if(isValueZero(_value2)==M_TRUE)return _value1;
	// MDH@24OCT2019: integer operations should be done using big integers, if the result is to be integer we map to M_LL_INVALID if the result is out of range!!!!
	//				we first do this for the add() binary operator, after this we're going to role this procedure out on the other binary operations!!!
	//				this means:
	//				1. comment out the next block
	//				2. function getBigintegerInteger() was created in order to map the sum big integer into the valid range of integers (if possible)
	/* no longer treat differently here
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Adding integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
	}
	*/
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _sumBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//			 
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger);
		Mbiginteger *_biginteger2=(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger);
		// outputBiginteger("Adding '",_biginteger1,"' and '");outputBiginteger(NULL,_biginteger2,"'.\n"); // DEBUG
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerboseDebugging())
				{q2outputBiginteger("Adding big integers '",_biginteger1,"'");q2outputBiginteger(" and '",_biginteger2,"'");}
			_sumBiginteger=owned_biginteger(__biginteger(),owner);
			if(_sumBiginteger!=NULL&&mp_add(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_sumBiginteger))!=MP_OKAY){
				FREE_BIGINTEGER(_sumBiginteger,owner);_sumBiginteger=NULL;
				outputError("Failed to add two big integers");
			}
			 // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerboseDebugging())
				q2outputBiginteger(" - Sum: '",_sumBiginteger,"'.\n");
		}else
			outputError("Failed to convert an integer to a big integer");
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//				but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(NULL==_sumBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llsum=getBigintegerInteger(_sumBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llsum!=M_LL_INVALID){
				FREE_BIGINTEGER(_sumBiginteger,owner);
				return _getIntegerValue(llsum);
			}
			outputWarning("Small integer sum out of range, will continue using big integer sum.");
		}
		return _getValueOfBiginteger(disowned_biginteger(_sumBiginteger,owner));
	}
	// if the first value is a text we should always do concatenation!!!!
	if(_value1->type==VT_TEXT){ // force string concatenation using the quote character in the Mvalue in the resulting text
		Mstring* _valueText=owned_string(__string(),owner);
		if(NULL==_valueText)return NULL;
		Mstring* p=_valueText;
		p=string_append_char(p,_value1->value._text->presuffix);
		p=string_append(p,_value1->value._text->_c);
		// MDH@17OCT2019: we can't use _getValueText() here, because _getValueText() will resolve escape sequences which we do NOT want here
		// MDH@28OCT2019: think twice this is only true when _value2 is also of type text
		if(_value2->type!=VT_TEXT){
			Mstring* _value2Text=owned_string(_getValueText(_value2,true,true),owner); // get the text representation of the second argument without quotes
			if(_value2Text!=NULL){p=string_append(p,string(_value2Text));FREE_STRING(_value2Text,owner);}
		}else // second argument also of type text
			p=string_append(p,_value2->value._text->_c);
		Mvalue* _value=(p!=NULL?_getTextValue(string(_valueText)):NULL);
		FREE_STRING(_valueText,owner);
		return _value;
	}
	// if either is a rational, compute the sum rational (NOTE or rationals disguised as decimals)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		if(_value1->type!=VT_RATIONAL)owned_rational(_rational1,owner);else if(_value2->type!=VT_RATIONAL)owned_rational(_rational2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		/////outputInfo("Adding two rationals.");
		Mrational* _sumRational=owned_rational(_getRationalSum(_rational1,_rational2),owner); // _qsum replaced by _getRationalSum that takes the deltas into account as well
		/////outputInfo("Rationals added!");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);else if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(amVerboseDebugging())
			outputInfo("Rational copies released.");
		Mvalue* _sumValue=NULL;
		if(_sumRational!=NULL){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_sumValue=_getValueOfDecimal(_getRationalDecimal(_sumRational,NULL));
				FREE_RATIONAL(_sumRational,owner);
			}else
				_sumValue=_getValueOfRational(disowned_rational(_sumRational,owner));
		}
		return _sumValue;
	}
	// if either is a decimal, compute the sum decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1,NULL),*_decimal2=getValueDecimal(_value2,NULL); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)owned_decimal(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)owned_decimal(_decimal2,owner);
		Mdecimal* _sumDecimal=owned_decimal(_getDecimalSum(_decimal1,_decimal2),owner); // _dadd replaced by _getDecimalSum that takes the repeating decimal digits into account as well
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(NULL==_sumDecimal)return NULL; // failed to create the sum for whatever reason
		return _getValueOfDecimal(disowned_decimal(_sumDecimal,owner));
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerboseDebugging())
			{q2outputValue("Adding integer/reals '",_value1,"'");q2outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1+ld2:M_LD_NAN);
	}
	/* MDH@28OCT2019: either real already dealt with above
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT||_value2->type==VT_TEXT)){
		// if the second argument is text, convert it to a real or integer number
		if(_value2->type==VT_TEXT){
			// are we going to convert it to an integer or a real????
			// NOTE a real has a period in the text, so use that
			char* valueText=_value2->value._text->_c;
			if(strchr(valueText,'.')!=NULL){ // a period 
				////////if(strlen(_valueText)==1)return _value1; // if a single period no need to actually add it unless someone want to change an integer in a real????
				////// we can use _strtold!!! long double ld=0;if(strlen(valueText)>1){char *endPtr=NULL;ld=strtold(valueText,&endPtr);if(endPtr==valueText){output("ERROR: Can't add '%s'.",valueText);return NULL;}} // failure
				_value2=_getFloatValue(_strtold(valueText,getNAR()));
			}else{ // no period
				_value2=_getIntegerValue(_strtoll(valueText,getNAI()));
			}
		}
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll+_value2->value._integer->ll);
		return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)+(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld));
	}
	*/
	return NULL;
}
/**
 * @brief returns the difference of \p _value2 and \p _value1
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* \p _value1 minus \p _value2
 */
Mvalue* Msubtract(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(amVerboseDebugging())
		{q2outputValue("Subtracting '",_value2,"'");q2outputValue(" from '",_value1,"'.\n");}
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,Msubtract,true);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,Msubtract,true);
	if(_value2->type==VT_ARRAY)return _appliedToArray(_value2->value._array,_value1,Msubtract,true);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,Msubtract,true);
	// if either is zero, result is easy to determine
	if(isValueZero(_value1)==M_TRUE)return Mneg(_value2);
	if(isValueZero(_value2)==M_TRUE)return _value1;
	if(amVerboseDebugging())
		{q2outputValue("Subtracting scalar '",_value2,"'");q2outputValue(" from scalar '",_value1,"'.\n");}
	/*
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Subtracting integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
	}
	*/
	// the other integer one could be a big integer in which case we return a big integer
	// the other integer one could be a big integer in which case we return a big integer
	// MDH@16DEC2020: if the second value is a time we can convert it to an integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER||_value1->type==VT_TIME)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER||_value2->type==VT_TIME)){
		Mbiginteger* _differenceBiginteger=NULL;
		// no need to use _getValueBiginteger because we know the source will be integer
		// NOTE _getBiginteger() was adjusted to return NULL in case ll equals M_LL_INVALID because in that case the result should also be M_LL_INVALID
		// CORRECTION: _getBiginteger() is also used in the I() function and it's a problem if NOT allowing to actually use NAI in computations (as the smallest possible integer)
		// DISCUSSION: M_LL_INVALID is a single long long value that is considered invalid like dividing by zero, or asking for the sign of an undefined real (= long double)
		//			 
		bool smallinteger1=(_value1->type==VT_INTEGER),smallinteger2=(_value2->type==VT_INTEGER);
		bool invalidinteger1=(smallinteger1&&_value1->value._integer->ll==M_LL_INVALID),invalidinteger2=(smallinteger2&&_value2->value._integer->ll==M_LL_INVALID);
		if(invalidinteger1||invalidinteger2)return _getIntegerValue(M_LL_INVALID); // if either integer is invalid return an invalid integer (which per definition will be small)
		// ASSERT both integers are considered valid (i.e. not invalid)
		Mbiginteger *_biginteger1=((_value1->type==VT_TIME
									?owned_biginteger(_getBiginteger(getTimeLongLong(_value1->value._time)),owner)
									:(smallinteger1?owned_biginteger(_getBiginteger(_value1->value._integer->ll),owner):_value1->value._biginteger)));
		Mbiginteger *_biginteger2=((_value2->type==VT_TIME
									?owned_biginteger(_getBiginteger(getTimeLongLong(_value2->value._time)),owner)
									:(smallinteger2?owned_biginteger(_getBiginteger(_value2->value._integer->ll),owner):_value2->value._biginteger)));
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerboseDebugging())
				{q2outputBiginteger("Subtracting big integers '",_biginteger1,"'");q2outputBiginteger(" and '",_biginteger2,"'");}
			_differenceBiginteger=owned_biginteger(__biginteger(),owner);
			if(_differenceBiginteger!=NULL&&mp_sub(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_differenceBiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_differenceBiginteger,owner);_differenceBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerboseDebugging())
				{q2outputBiginteger(" - Difference: '",_differenceBiginteger,"'.\n");}
		}else
			outputError("Failed to convert an integer to a big integer");
		if(_value1->type==VT_TIME)smallinteger1=true; // MDH@16DEC2020: from here treat time value also as a small integer
		if(_value2->type==VT_TIME)smallinteger2=true; // MDH@16DEC2020: from here treat time value also as a small integer
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner); 
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//				but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(NULL==_differenceBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long lldifference=getBigintegerInteger(_differenceBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(lldifference!=M_LL_INVALID){FREE_BIGINTEGER(_differenceBiginteger,owner);return _getIntegerValue(lldifference);}
			outputWarning("Small integer difference out of range, will continue using big integer difference.");
		}
		return _getValueOfBiginteger(disowned_biginteger(_differenceBiginteger,owner));
	}
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2); // OOPS careful here, _getValueRational might construct a new rational or what????
		if(_value1->type!=VT_RATIONAL)owned_rational(_rational1,owner);
		else 
		if(_value2->type!=VT_RATIONAL)owned_rational(_rational2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(amVerboseDebugging())
			{q2outputRational("Computing the difference of rational '",_rational1,"'");outputRational(" and rational '",_rational2,"'.\n");}
		Mrational* _differenceRational=owned_rational(_getRationalDifference(_rational1,_rational2),owner); // _qsubtract replaced by _getRationalDifference() which takes deltas into account as well
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		else 
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		Mvalue* _differenceValue=NULL;
		if(_differenceRational!=NULL){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_differenceValue=_getValueOfDecimal(_getRationalDecimal(_differenceRational,NULL));
				FREE_RATIONAL(_differenceRational,owner);
			}else
				_differenceValue=_getValueOfRational(disowned_rational(_differenceRational,owner));
		}
		return _differenceValue;
	}
	// if either is a decimal, compute the difference decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1,NULL),*_decimal2=getValueDecimal(_value2,NULL); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)owned_decimal(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)owned_decimal(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		Mdecimal* _differenceDecimal=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner); // _dsub replaced by _getDecimalDifference which takes repeating decimal digits into account as well
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(NULL==_differenceDecimal)return NULL; // failed to create the sum for whatever reason
		return _getValueOfDecimal(disowned_decimal(_differenceDecimal,owner));
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerboseDebugging())
			{q2outputValue("Subtracting integer/reals '",_value1,"'");q2outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1-ld2:M_LD_NAN);
	}
	/* MDH@28OCT2019: now obsolete
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER)return _getIntegerValue(_value1->value._integer->ll-_value2->value._integer->ll);
		return _getFloatValue((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)-(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld));
	}
	*/
	return NULL;
}
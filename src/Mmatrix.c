// MDH@04APR2023: matrices are two-dimensional arrays with data in an associated Marray

#include "Mmatrix.h"

extern unsigned long long M_MODULE_DEBUGGING;
extern long long M_LL_INVALID;
extern long long M_TRUE;
extern long long M_FALSE;
extern long double M_LD_NAN;
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types

// MDH@22NOV2020
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_MATRIX,id};}

/*
Miterator** getMatrixIterators(Mmatrix* matrix){
	return NULL;
}
*/


/**
 * @brief returns M_TRUE if \p array is a matrix i.e. a two-dimensional array, M_FALSE if it is not
 * @details returns M_LL_INVALID if \p array is NULL
 * @param array 
 * @return long long M_TRUE if \p is a matrix, M_FALSE or M_LL_INVALID otherwise
 */
long long isAMatrix(Marray* array){
	if(array!=NULL){
		if(array->numberOfDimensionsLeft==1)return M_TRUE;
		output("Number of dimensions left: %lld.\n",array->numberOfDimensionsLeft);
		return M_FALSE;
	}
	return M_LL_INVALID;
	// replacing: return(NULL==array?M_LL_INVALID:(array->numberOfDimensionsLeft==1?M_TRUE:M_FALSE));
}
/**
 * @brief returns the number of rows of matrix \p array
 * 
 * @param array 
 * @return long long the number of rows in matrix \p array
 */
long long getNumberOfMatrixRows(Marray* array){
	return(isAMatrix(array)==M_TRUE?array->numberOfElements:M_LL_INVALID);
}
/**
 * @brief returns the number of columns of matrix \p array
 * @details returns M_LL_INVALID if \p is not a matrix
 * @param array 
 * @return long long the number of columns of matrix \p array
 */
long long getNumberOfMatrixColumns(Marray* array){
	return(isAMatrix(array)==M_TRUE?array->values[0]->value._array->numberOfElements:M_LL_INVALID);
}

/**
 * @brief returns M_TRUE if \p array is a numeric matrix, M_FALSE otherwise
 * @details returns returns M_LL_INVALID if \p array is not a matrix
 * @param array 
 * @return long long M_TRUE if \p array is a numeric matrix, M_FALSE or M_LL_INVALID otherwise
 */
long long isANumericMatrix(Marray* array){
	// if array is not a matrix, we return M_LL_INVALID
	if(isAMatrix(array)==M_TRUE){
		Mvalue* _value=array->values[0];
		if(_value!=NULL&&_value->type==VT_ARRAY){
			// _value->value._array->values[0]->type;
			output("Type of array element: %c.\n",MUTABLEVALUETYPECHARS[_value->value._array->values[0]->type]); // replacing:_value->value._array->valuetype);
			// I think we have to look at the valuetype of the first element
			if(isANumericValuetype(_value->value._array->values[0]->type))return M_TRUE;
			//////output("Type of array (%c) not numeric.\n",getValueTypeCharacter(_value->value._array->values[0]->type)); // replacing:_value->value._array->valuetype);
			return M_FALSE;
		}
		output("Not an array!\n");
	}else
		output("Not a matrix!\n");
	return M_LL_INVALID;
}

static Mvaluetype getMatrixElementValuetype(Marray* array){
	return(array->values[0]->value._array->values[0]->type);
}

/**
 * @brief returns the matrix product of two-dimensional arrays \p array1 and \p array2
 * @details the number of columns of \p array1 should equal the number of rows of \p array2
 *          both arrays need to be of a numeric type
 * @param array1 
 * @param array2 
 * @return Marray* the matrix product of two-dimensional numeric array \p array1 and \p array2
 */
Marray* matrixproduct(Marray* array1,Marray* array2){Mallocationowner owner=getOwner(__LINE__);
	assert(isANumericMatrix(array1)==M_TRUE&&isANumericMatrix(array2)==M_TRUE);
	long long array1cols=getNumberOfMatrixColumns(array1),array2rows=getNumberOfMatrixRows(array2);
	if(array1cols==array2rows){
		// we need to multiply rows of array1 with columns of array2
		// it's easy to iterate the rows of array1
		Mvaluetype productValuetype=MAX(getMatrixElementValuetype(array1),getMatrixElementValuetype(array2));
		Mvalue* fillValue=getValueZeroOfType(productValuetype);
		long long array1rows=getNumberOfMatrixRows(array1),array2columns=getNumberOfMatrixColumns(array2);
		// TODO getValueOfArray() should free the created array when failing!!!
		Mvalue* _matrixproductRowValue=_getValueOfArray(_getArray(NULL,array2columns,fillValue));
		if(_matrixproductRowValue!=NULL){
			// TODO perhaps _getArray takes care of this??????
			if(_matrixproductRowValue->value._array->valuetype!=productValuetype){
				_matrixproductRowValue->value._array->valuetype=productValuetype;
				outputBug("_getArray() does not assign the right element value type!");
			}
			Marray* _matrixproduct=owned_array(_getArray(NULL,array1rows,_matrixproductRowValue),owner);
			if(_matrixproduct!=NULL){
				_matrixproduct->numberOfDimensionsLeft=1; // so it's considered a matrix!!!!
				// iterate over the rows of array1
				for(long long rowIndex=0;rowIndex<array1rows;rowIndex++){
					Mvalue** productRowArrayValues=_matrixproduct->values[rowIndex]->value._array->values; // the product array to fill
					Mvalue** rowArrayValues=array1->values[rowIndex]->value._array->values;
					for(long long colIndex=0;colIndex<array2columns;colIndex++){
						Mvalue* sumproductValue=productRowArrayValues[colIndex]; // which will be zero!!!
						for(long long elementIndex=0;elementIndex<array1cols;elementIndex++)
							sumproductValue=add(sumproductValue,multiply(rowArrayValues[elementIndex],array2->values[elementIndex]->value._array->values[colIndex]));
						assignValue(&productRowArrayValues[colIndex],sumproductValue);
					}
				}
				return disowned_array(_matrixproduct,owner);
			}
		}
		outputError("Failed to create the product matrix");
	}else
		outputError("Matrices cannot be multiplied: the number of rows and columns do not match");
	return NULL;
}

/**
 * @brief returns the inverse of matrix \p array1
 * 
 * @param array1 
 * @return Marray* the inverse of matrix \p array1
 */
Marray* matrixinverse(Marray* array1){
	if(isANumericMatrix(array1)==M_TRUE){

	}
	return NULL;
}

// multiply and divide operations we need in matrixproduct and matrixinverse
/**
 * @brief returns the product of \p _value1 and \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the product of \p _value1 and \p _value2
 */
Mvalue* multiply(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	// two-dimensional numeric matrices should be matrix multiplied
	if(_value1->type==VT_ARRAY&&_value2->type==VT_ARRAY)
		if(isANumericMatrix(_value1->value._array)==M_TRUE&&isANumericMatrix(_value2->value._array)==M_TRUE)
			return _getValueOfArray(matrixproduct(_value1->value._array,_value2->value._array));
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,multiply,true);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,multiply,true);
	if(_value2->type==VT_ARRAY)return _appliedToArray(_value2->value._array,_value1,multiply,true);
	if(_value2->type==VT_LIST)return _appliedToList(_value2->value._list,_value1,multiply,true);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE||isValueOne(_value1)==M_TRUE)return _value2;
	// MDH@26OCT2019: adapted from dealing with any integer type from add()
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _productBiginteger=NULL;
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
		// replacing: Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1!=NULL&&_biginteger2!=NULL){
			if(amVerboseDebugging())
				{outputBiginteger("Multiplying big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'");}
			_productBiginteger=owned_biginteger(__biginteger(),owner);
			if(_productBiginteger!=NULL&&mp_mul(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2),MP_INT_POINTER(_productBiginteger))!=MP_OKAY)
			{FREE_BIGINTEGER(_productBiginteger,owner);_productBiginteger=NULL;} // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
			if(amVerboseDebugging())
			{outputBiginteger(" - Product: '",_productBiginteger,"'.\n");}
		}else
			outputError("Failed to convert a small integer to a big integer");
		if(smallinteger1)FREE_BIGINTEGER(_biginteger1,owner);
		if(smallinteger2)FREE_BIGINTEGER(_biginteger2,owner);
		// MDH@24OCT2019: now we're going to try to convert the sum back to an integer if we can
		//				but if we can't don't
		if(smallinteger1||smallinteger2){ // we could decide to try to keep the value in range if at least one of the integers is small (instead of demanding both are small integers)
			// if computing the sum failed return the invalid (small) integer (to indicate a missing result)
			if(NULL==_productBiginteger)return _getIntegerValue(M_LL_INVALID);
			long long llproduct=getBigintegerInteger(_productBiginteger); // will return M_LL_INVALID when _sumBiginteger equals NULL (which we want to exclude)
			// if we do NOT have a sum big integer or the sum big integer is in range ()
			if(llproduct!=M_LL_INVALID){FREE_BIGINTEGER(_productBiginteger,owner);return _getIntegerValue(llproduct);}
			outputWarning("Small integer product out of range, will continue using big integer product.");
		}
		return _getValueOfBiginteger(disowned_biginteger(_productBiginteger,owner));
	}
	/* replacing:
	// if both are integers, the result should be integer as well!!!
	if(_value1->type==VT_INTEGER&&_value2->type==VT_INTEGER){
			if(amVerbose())output("Multiplying integers '%lld' and '%lld'.\n",_value1->value._integer->ll,_value2->value._integer->ll);
			return _getIntegerValue(_value1->value._integer->ll*_value2->value._integer->ll);
	}
	// the other integer one could be a big integer in which case we return a big integer
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		Mbiginteger* _productBiginteger=NULL;
		Mbiginteger *_biginteger1=_getValueBiginteger(_value1),*_biginteger2=_getValueBiginteger(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_biginteger1&&_biginteger2){
			if(amVerbose()){outputBiginteger("Multiplying big integers '",_biginteger1,"'");outputBiginteger(" and '",_biginteger2,"'.\n");}
			_productBiginteger=__biginteger();
			if(!_productBiginteger)outputError("Failed to create the product big integer");else
			if(mp_mul(_biginteger1,_biginteger2,_productBiginteger)!=MP_OKAY){
				FREE_BIGINTEGER(_productBiginteger);_productBiginteger=NULL;outputError("Failed to multiply two big integers");
			}else
			if(amVerbose())outputBiginteger("Big integer product: '",_productBiginteger,"'.\n");
			 // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
		}else
			outputError("Failed to create two helper big integers");
		if(_value1->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger1);else if(_value2->type!=VT_BIGINTEGER)FREE_BIGINTEGER(_biginteger2); // after adding the two rationals we do not need the newly created rationals anymore
		return _getValueOfBiginteger(disowned_biginteger(_productBiginteger,true);
	}
	*/
	// if either is rational do a rational multiplication
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		if(amVerbose()){outputValue("Multiplying rationals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		Mrational *_rational1=getValueRational(_value1),*_rational2=getValueRational(_value2);
		if(_value1->type!=VT_RATIONAL)owned_rational(_rational1,owner);else if(_value2->type!=VT_RATIONAL)owned_rational(_rational2,owner); // after dividing the two rationals we do not need the newly created rationals anymore
		Mrational* _productRational=owned_rational(_getRationalProduct(_rational1,_rational2),owner); // _qproduct replaced by _getRationalProduct as defined in Mrational.h/c
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);else if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner); // after dividing the two rationals we do not need the newly created rationals anymore
		Mvalue* _productValue=NULL;
		if(_productRational!=NULL){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_productValue=_getValueOfDecimal(_getRationalDecimal(_productRational));
				FREE_RATIONAL(_productRational,owner);
			}else
				_productValue=_getValueOfRational(disowned_rational(_productRational,owner));
		}
		return _productValue;
	}
	// if either is a decimal, compute the product decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		if(amVerboseDebugging())
			{outputValue("Multiplying decimals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)owned_decimal(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)owned_decimal(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		Mdecimal* _productDecimal=owned_decimal(_getDecimalProduct(_decimal1,_decimal2),owner); // _dmul replaced by _getDecimalProduct which should be able to multiply any two decimals (not just the pure decimals)
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		if(NULL==_productDecimal)return NULL; // failed to create the sum for whatever reason
		return _getValueOfDecimal(disowned_decimal(_productDecimal,owner));
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerboseDebugging())
		{outputValue("Multiplying integer/reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1*ld2:M_LD_NAN);
	}
	return NULL;
}

// MDH@07JUN2019: when two integers are presented to divide instead of actually computing the division we can store the division as a rational (so we kind of have a slow evaluation of the division, and we maintain accuracy as long as possible)
/**
 * @brief returns the quotient of \p value1 and \p value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the quotient of \p value1 and \p value2
 */
Mvalue* divide(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type==VT_ARRAY&&_value2->type==VT_ARRAY)
		if(isANumericMatrix(_value1->value._array)==M_TRUE&&isANumericMatrix(_value2->value._array)==M_TRUE)
			return _getValueOfArray(matrixproduct(_value1->value._array,matrixinverse(_value2->value._array)));
	if(_value1->type==VT_ARRAY)return _appliedToArray(_value1->value._array,_value2,divide,false);
	if(_value1->type==VT_LIST)return _appliedToList(_value1->value._list,_value2,divide,false);
	if(_value2->type==VT_ARRAY)return _appliedToArray2(_value1,_value2->value._array,divide,false);
	if(_value2->type==VT_LIST)return _appliedToList2(_value1,_value2->value._list,divide,false);
	if(isValueZero(_value1)==M_TRUE||isValueOne(_value2)==M_TRUE)return _value1;
	if(isValueZero(_value2)==M_TRUE)return NULL; // TODO shouldn't we return infinity?????
	// MDH@26OCT2019: dealing with any integer conform as we did in the other binary operators
	//				NO dividing integers should result in a rational so we can keep the accuracy
	// NOT replacing:
	// integer divisions are not computed but stored in rational format (without a delta to not suggest that the division is decimal)
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){ // both are integer
		Mbiginteger* _numerator=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _denominator=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		// if the denominator is negative, both the numerator and denominator should be negated (should this be part of the normalization procedure?), theoretically storing the sign separate from the big integers in a rational could also be the way to go
		// so when the sign of the two big integers is different, the rational is negative, otherwise it is positive and _getRational would store the absolute values of the big integer
		// if _getRational would take care of negating the numerator and denominator it would have to free the passed in big integers (if so requested)
		Mrational* _rational=owned_rational(_getRational(_numerator,_denominator,M_LD_NAN,true),owner); // free num/den when failing to bind
		FREE_BIGINTEGER(_numerator,owner);FREE_BIGINTEGER(_denominator,owner);
		return _getValueOfRational(disowned_rational(_rational,owner)); // when failing to bind _rational to a value, free it as well
	}
	// if one of them is a rational do a rational division
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		Mrational *_rational1=getValueRational(_value1),
							*_rational2=getValueRational(_value2);
		if(_value1->type!=VT_RATIONAL)owned_rational(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)owned_rational(_rational2,owner);	
		Mrational* _quotientRational=owned_rational(_getRationalQuotient(_rational1,_rational2),owner); // _qdivide replaced by _getRationalQuotient (as defined in Mrational.h/c)
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);else 
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner); // after dividing the two rationals we do not need the newly created rationals anymore
		Mvalue* _quotientValue=NULL;
		if(_quotientRational!=NULL){
			if(_value1->type==VT_DECIMAL&&_value2->type==VT_DECIMAL){
				_quotientValue=_getValueOfDecimal(_getRationalDecimal(_quotientRational));
				FREE_RATIONAL(_quotientRational,owner);
			}else
				_quotientValue=_getValueOfRational(disowned_rational(_quotientRational,owner));
		}
		return _quotientValue;
	}
	// if either is a decimal, compute the quotient decimal
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		Mdecimal *_decimal1=getValueDecimal(_value1),*_decimal2=getValueDecimal(_value2); // OOPS careful here, _getValueDecimal would make a copy which we do not want here!!!!
		if(_value1->type!=VT_DECIMAL)owned_decimal(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)owned_decimal(_decimal2,owner); 
		// after adding the two rationals we do not need the newly created rationals anymore		
		Mdecimal* _divideDecimal=owned_decimal(_getDecimalQuotient(_decimal1,_decimal2),owner); // _ddiv now replaced by _getDecimalQuotient which should be able to divide any two decimals not just the pure once!!!!!
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);else 
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner); // after adding the two rationals we do not need the newly created rationals anymore
		return _getValueOfDecimal(disowned_decimal(_divideDecimal,owner));
	}
	// if either is a real
	if(_value1->type==VT_FLOAT||_value2->type==VT_FLOAT){
		if(amVerbose()){outputValue("Dividing (as) reals '",_value1,"'");outputValue(" and '",_value2,"'.\n");}
		long double ld1=getValueLongDouble(_value1),ld2=getValueLongDouble(_value2);
		return _getFloatValue(isLongDoubleUndefined(ld1)==M_FALSE&&isLongDoubleUndefined(ld2)==M_FALSE?ld1/ld2:M_LD_NAN);
	}
	/* replacing:
	// always real divide
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT)){
		long double ld1=(_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld); // TODO casting to a long double is perhaps not the best way?
		long double ld2=(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld); // TODO casting to a long double is perhaps not the best way?
		return _getFloatValue(ld1/ld2);
	}
	*/
	return NULL;
}
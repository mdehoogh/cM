// MDH@04APR2023: matrices are two-dimensional arrays with data in an associated Marray

#include "Mmatrix.h"

extern unsigned long long M_MODULE_DEBUGGING;
extern long long M_LL_INVALID;
extern long long M_TRUE;
extern long long M_FALSE;
extern long double M_LD_NAN;
extern long long M_DP;
extern const char* MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern Mdecimalcontext* M_DECIMALCONTEXT;

// MDH@22NOV2020
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_MATRIX,id};}

/*
Miterator** getMatrixIterators(Mmatrix* matrix){
	return NULL;
}
*/
/* MDH@01MAY2023: matrices are implemented using 2-dim array, so no need for separate matrix definition
// module (static) helper structures and functions
// operations using matrices can be performed on either decimals or rationals
// depending on the original data type of the matrix
typedef enum Mmatrixtype { MT_UNKNOWN,MT_DECIMAL,MT_RATIONAL } Mmatrixtype;
typedef union Mmatrixunion{
	Mdecimal** decimals;
	Mrational** rationals;
}Mmatrixunion;
typedef struct Mmatrix{
	unsigned long long rows,columns;
	Mmatrixtype type;
	Mmatrixunion matrixunion;
}Mmatrix;

static Mmatrix getMatrix(Mvalue* matrixValue){
	Mmatrix matrix={};
	return matrix;
}
*/
/**
 * @brief returns M_TRUE if \p _value1 is smaller than \p _value2, M_FALSE otherwise
 * 
 * @param _value1 
 * @param _value2 
 * @return long long 
 */
/*static*/ long long smallerthan(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	// ASSERT do not call when either is a list
	if(NULL==_value1&&NULL==_value2)return M_FALSE; // both NULL, so equal, and therefore not smaller than
	if(NULL==_value1||NULL==_value2)return(_value1!=NULL?M_TRUE:M_FALSE); // if _value1 is NULL yes always smaller, otherwise _value2 is NULL and _value1 is never smaller
	// MDH@02NOV2020: comparing texts
	if(_value1->type==VT_TEXT||_value2->type==VT_TEXT){
		Mstring *_value1text=owned_string(_getValueText(_value1,true),owner),*_value2text=owned_string(_getValueText(_value2,true),owner);
		int result=(_value1text!=NULL&&_value2text!=NULL?strcmp(string(_value1text),string(_value2text)):(_value1text!=NULL?1:(_value2text!=NULL?-1:0))); // NULL is always supposedly smaller
		FREE_STRING(_value1text,owner);FREE_STRING(_value2text,owner);
		return(result<0?M_TRUE:M_FALSE); 
	}
	if((_value1->type==VT_INTEGER||_value1->type==VT_FLOAT)&&(_value2->type==VT_INTEGER||_value2->type==VT_FLOAT))
		return((_value1->type==VT_INTEGER?_value1->value._integer->ll:_value1->value._float->ld)<(_value2->type==VT_INTEGER?_value2->value._integer->ll:_value2->value._float->ld)?M_TRUE:M_FALSE);
	if((_value1->type==VT_INTEGER||_value1->type==VT_BIGINTEGER)&&(_value2->type==VT_INTEGER||_value2->type==VT_BIGINTEGER)){
		// creating two intermediate big integers that need to be freed asap
		Mbiginteger* _biginteger1=owned_biginteger(_value1->type==VT_INTEGER?_getBiginteger(_value1->value._integer->ll):_getBigintegerCopy(_value1->value._biginteger),owner);
		Mbiginteger* _biginteger2=owned_biginteger(_value2->type==VT_INTEGER?_getBiginteger(_value2->value._integer->ll):_getBigintegerCopy(_value2->value._biginteger),owner);
		long long llsmallerthan=(_biginteger1&&_biginteger2?(mp_cmp(MP_INT_POINTER(_biginteger1),MP_INT_POINTER(_biginteger2))==MP_LT?M_TRUE:M_FALSE):M_LL_INVALID); // if either is not zero, the result is 1 otherwise 0, NOTE using || is better than using &&???
		FREE_BIGINTEGER(_biginteger1,owner);FREE_BIGINTEGER(_biginteger2,owner); // free the created copies
		return llsmallerthan;
	}
	// MDH@23OCT2019: if we can rationalize at least one of the values, we should work with rationals (so we get the highest possible accuracy in the comparison)
	if((_value1->type==VT_RATIONAL||(_value1->type==VT_DECIMAL&&_value1->value._decimal->repeating>0))||(_value2->type==VT_RATIONAL||(_value2->type==VT_DECIMAL&&_value2->value._decimal->repeating>0))){
		long long result=M_LL_INVALID;
		Mrational *_rational1=owned_rational(_getValueRational(_value1),owner),
				 			*_rational2=owned_rational(_getValueRational(_value2),owner);
		if(_rational1!=NULL&&_rational2!=NULL){
			Mrational* _rationalDifference=owned_rational(_getRationalDifference(_rational1,_rational2),owner);
			if(_rationalDifference!=NULL){
				if(amVerbose())outputRational("Difference in determining whether a rational is smaller than another rational: '",_rationalDifference,"'.\n");
				result=isRationalNegative(_rationalDifference);
				FREE_RATIONAL(_rationalDifference,owner);
			}else
				outputError("Failed to compute the difference of two rationals");
		}else
			outputError("Failed to convert comparison operator arguments to rationals");
		if(_value1->type!=VT_RATIONAL)FREE_RATIONAL(_rational1,owner);
		if(_value2->type!=VT_RATIONAL)FREE_RATIONAL(_rational2,owner);
		return result;
	}
	if(_value1->type==VT_DECIMAL||_value2->type==VT_DECIMAL){
		// creating two intermediate decimals that need to be freed asap
		long long result=M_LL_INVALID;
		Mdecimal 	*_decimal1=owned_decimal(_getValueDecimal(_value1),owner),
							*_decimal2=owned_decimal(_getValueDecimal(_value2),owner);
		if(_decimal1!=NULL&&_decimal2!=NULL){
			Mdecimal* _decimalDifference=owned_decimal(_getDecimalDifference(_decimal1,_decimal2),owner);
			if(_decimalDifference!=NULL){
				if(amVerbose())
					outputDecimal("Difference in determining whether a decimal is smaller than another decimal: '",_decimalDifference,"'.\n");
				result=isDecimalNegative(_decimalDifference);
				FREE_DECIMAL(_decimalDifference,owner);
			}else
				outputError("Failed to compute the difference of two decimals");
		}else
			outputError("Failed to convert comparison arguments to decimals");
		if(_value1->type!=VT_DECIMAL)FREE_DECIMAL(_decimal1,owner);
		if(_value2->type!=VT_DECIMAL)FREE_DECIMAL(_decimal2,owner);
		return result;
	}
	return M_LL_INVALID;
}

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

static bool allValuesAreNumeric(Mvalue** values,unsigned long long numberOfValues){
	while(numberOfValues>0){
		Mvalue* value=values[--numberOfValues];
		if(value==NULL||!isANumericValuetype(value->type))
			return false;
	}
	return true;
}
/**
 * @brief returns M_TRUE if \p array is a numeric matrix, M_FALSE otherwise
 * @details returns M_LL_INVALID if \p array is not a matrix
 *          if numberOfDimensionsLeft field of \p array is 1 is assumed to be a numeric matrix
 * @param array 
 * @return long long M_TRUE if \p array is a numeric matrix, M_FALSE or M_LL_INVALID otherwise
 */
long long isANumericMatrix(Marray* array,bool update){
	if(array!=NULL){
		// let's take a look at the numberOfDimensionsLeft
		long long dimensionsLeft=array->numberOfDimensionsLeft;
		if(dimensionsLeft!=1){
			if(dimensionsLeft==0){ // we'd have to check
				//if(amVerbose())
				output("Checking if an array is a matrix.\n");
				// it requires two dimensions and all cells should be integer!!!
				// this means that all values in the array must be arrays themselves
				// and have the same number of elements to start with
				Mvalue* firstArrayValue=array->values[0];
				if(firstArrayValue!=NULL&&firstArrayValue->type==VT_ARRAY){
					// check all the array values for being numeric
					unsigned long long numberOfValues=firstArrayValue->value._array->numberOfElements;
					if(isANumericValuetype(firstArrayValue->value._array->valuetype)==M_TRUE||
							allValuesAreNumeric(firstArrayValue->value._array->values,numberOfValues)){
						// the first value is an array with numberOfValues numeric values
						// and so should all the other values as well
						unsigned long long elementIndex=array->numberOfElements;
						while(--elementIndex>0){
							Mvalue* value=array->values[elementIndex];
							if(value==NULL||
									value->type!=VT_ARRAY||
									value->value._array->numberOfElements!=numberOfValues||
									(isANumericValuetype(value->value._array->valuetype)!=M_TRUE&&
										!allValuesAreNumeric(value->value._array->values,numberOfValues)))
								break;
						}	
						// if not broken out of the loop due to not matching array element
						if(elementIndex==0)dimensionsLeft=-1;else outputError("Not all row elements numeric");
					}else
						outputError("Not all array elements in the first matrix row are numeric");
				}else
					outputError("Not a two-dimensional array");
			}
			if(dimensionsLeft==-1)
				if(update)
					array->numberOfDimensionsLeft=1;
		}
		return(llabs(dimensionsLeft)==1?M_TRUE:M_FALSE);
	}
	return M_LL_INVALID;
}
/* replacing: 
long long isANumericMatrix(Marray* array){
	// if array is not a matrix, we return M_LL_INVALID
	if(isAMatrix(array)==M_TRUE){
		Mvalue* _value=array->values[0];
		if(_value!=NULL&&_value->type==VT_ARRAY){
			// _value->value._array->values[0]->type;
			output("Type of array element: %c.\n",MUTABLEVALUETYPECHARS[_value->value._array->values[0]->type]); // replacing:_value->value._array->valuetype);
			// I think we have to look at the valuetype of the first element
			if(isANumericValuetype(_value->value._array->values[0]->type)==M_TRUE)return M_TRUE;
			//////output("Type of array (%c) not numeric.\n",getValueTypeCharacter(_value->value._array->values[0]->type)); // replacing:_value->value._array->valuetype);
			return M_FALSE;
		}
		output("Not an array!\n");
	}else
		output("Not a matrix!\n");
	return M_LL_INVALID;
}
*/

/**
 * @brief returns the value type of elements of matrix \p array
 * 
 * @param array 
 * @return Mvaluetype the value type of elements of matrix \p array
 */
static Mvaluetype getMatrixElementValuetype(Marray* array){
	return(array->values[0]->value._array->values[0]->type);
}

/**
 * @brief returns a copy of matrix array
 * 
 * @param array 
 * @return Marray* 
 */
static Marray* matrixcopy(Marray* array){
	return _getArrayCopy(array);
}

/**
 * @brief returns the matrix product of two-dimensional arrays \p array1 and \p array2
 * @details the number of columns of \p array1 should equal the number of rows of \p array2
 *          both arrays need to be of a numeric type
 * @param array1 
 * @param array2 
 * @return Marray* the matrix product of two-dimensional numeric array \p array1 and \p array2
 */
static Marray* matrixproduct(Marray* array1,Marray* array2){Mallocationowner owner=getOwner(__LINE__);
	//// assert(isANumericMatrix(array1)==M_TRUE&&isANumericMatrix(array2)==M_TRUE);
	long long array1cols=getNumberOfMatrixColumns(array1),array2rows=getNumberOfMatrixRows(array2);
	if(array1cols==array2rows){
		// MDH@01MAY2023: there's no need to determine the productValuetype per se but we use it to get the highest accuracy zero to start with
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
						for(long long elementIndex=0;elementIndex<array1cols;elementIndex++){
							output("Adding the product of ");
							outputValue(NULL,rowArrayValues[elementIndex],NULL);
							output(" and ");
							outputValue(NULL,array2->values[elementIndex]->value._array->values[colIndex],NULL);
							outputValue(" to ",sumproductValue,".\n");
							sumproductValue=add(sumproductValue,multiply(rowArrayValues[elementIndex],array2->values[elementIndex]->value._array->values[colIndex]));
						}
						output("Product value at cell (%lld,%lld)",rowIndex,colIndex);
						outputValue(": ",sumproductValue,".\n");
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
 * @brief returns a square matrix with \p numberOfDiagonalValues of \p diagonalValues along the diagonal 
 * 
 * @param matrix_diagonal 
 * @return Marray* the diagonal square matrix
 */
Marray* diagonalmatrix(Mvalue** diagonalValues,long long numberOfValues){Mallocationowner owner=getOwner(__LINE__);
	if(diagonalValues!=NULL&&numberOfValues>0){
		// every row is different but starts with being zero
		Mvaluetype valuetype=diagonalValues[0]->type;
		Mvalue* zeroOfTypeValue=getValueZeroOfType(valuetype);
		///Mvalue* oneOfTypeValue=getValueOneOfType(valuetype);
		Marray* _row=OWNED_ARRAY(_getArray(NULL,numberOfValues,zeroOfTypeValue),owner);
		Marray* _rows=OWNED_ARRAY(_getArray(NULL,numberOfValues,_getValueOfArray(DISOWNED_ARRAY(_row,owner))),owner);
		// fill the diagonal of _rows
		for(long long rowIndex=0;rowIndex<numberOfValues;rowIndex++){
			Marray* row=_rows->values[rowIndex]->value._array;
			assignValue(&row->values[rowIndex],diagonalValues[rowIndex]);
		}
		return DISOWNED_ARRAY(_rows,owner);
	}
	return NULL;
}

// convenience method to obtain the wrapped mpd_context pointer
/**
 * @brief returns the mpd_context wrapped in M_DECIMAL_CONTEXT
 * 
 * @return mpd_context_t* the mpd_context wrapped in M_DECIMAL_CONTEXT
 */
mpd_context_t* get_default_mpd_context(){return(M_DECIMALCONTEXT!=NULL?M_DECIMALCONTEXT->mpd_context:NULL);}

/**
 * @brief returns the 1 value of the given \p valuetype
 * 
 * @param valuetype 
 * @return Mvalue* the 1 value in the given value type \p valuetype
 */
Mvalue* getValueOneOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
	switch(valuetype){
		case VT_TIME:
		case VT_INTEGER: return _getIntegerValue(1);
		case VT_BIGINTEGER: return _getValueOfBiginteger(_getBiginteger(1));
		case VT_FLOAT: return _getFloatValue(1.0);
		case VT_RATIONAL: return _getValueOfRational(_getRational(_getBiginteger(1),NULL,M_LD_NAN,false));
		case VT_DECIMAL: return _getValueOfDecimal(_getDecimal(__mpd(get_default_mpd_context(),1),M_DP,0,true));
		default:break;
	}
	return NULL;
}

/**
 * @brief returns the inverse of matrix \p array1
 * 
 * @param array1 
 * @return Marray* the inverse of matrix \p array1
 */
static Marray* matrixinverse(Marray* array){Mallocationowner owner=getOwner(__LINE__);
	///////assert(isANumericMatrix(array)==M_TRUE);
	long long numberOfArrayRows=getNumberOfMatrixRows(array);
	if(numberOfArrayRows==getNumberOfMatrixColumns(array)){
		Marray* _arrayCopy=OWNED_ARRAY(_getArrayCopy(array),owner); // TODO DONE needs to be owned at some point 
		if(_arrayCopy!=NULL){
			Marray* unityMatrix=NULL;
			// create an array of ones
			Mvalue* oneOfTypeValue=getValueOneOfType(getMatrixElementValuetype(array));
			Marray* diagonalOfOnes=OWNED_ARRAY(_getArray(NULL,numberOfArrayRows,oneOfTypeValue),owner);
			if(diagonalOfOnes!=NULL){
				// TODO perhaps diagonalOfOnes needs to be freed anyway!!!!
				unityMatrix=OWNED_ARRAY(diagonalmatrix(diagonalOfOnes->values,numberOfArrayRows),owner);
				if(unityMatrix!=NULL){
					// ok, ready to iterate over the columns of the 
					Mvalue** arrayRows=_arrayCopy->values;
					Mvalue** unityMatrixRows=unityMatrix->values;
					long long colIndex;
					// source: geeksforgeeks.org
					// 1. interchange the row of matrix
					for(long long rowIndex=numberOfArrayRows-1;rowIndex>0;rowIndex--){
						// can't compare using < because we have to compare the values bro'
						if(smallerthan(arrayRows[rowIndex-1]->value._array->values[0],arrayRows[rowIndex]->value._array->values[0])==M_TRUE){
							Mvalue* tempRow=arrayRows[rowIndex];
							arrayRows[rowIndex]=arrayRows[rowIndex-1];
							arrayRows[rowIndex-1]=tempRow;
							// don't forget to do this on the unity matrix as well
							tempRow=unityMatrixRows[rowIndex];
							unityMatrixRows[rowIndex]=unityMatrixRows[rowIndex-1];
							unityMatrixRows[rowIndex-1]=tempRow;
						}
					}
					outputArray("Rows rearranged: ",_arrayCopy,"\n");
					outputArray("\tUnity matrix: ",unityMatrix,"\n");
					// 2. replace a row by sum of itself and another a constant multiple of another row
					for(long long colIndex=0;colIndex<numberOfArrayRows;colIndex++){
						output("Column=%lld.\n",colIndex);
						for(long long rowIndex=0;rowIndex<numberOfArrayRows;rowIndex++){
							output("\tRow=%lld.\n",rowIndex);
							if(colIndex!=rowIndex){
								Mvalue* tempValue=divide(
													arrayRows[rowIndex]->value._array->values[colIndex],
													arrayRows[colIndex]->value._array->values[colIndex]);
								outputValue("\t\tTemp value: ",tempValue,"\n");
								for(long long elementIndex=0;elementIndex<numberOfArrayRows;elementIndex++){
									assignValue(
										&arrayRows[rowIndex]->value._array->values[elementIndex],
										subtract(
											arrayRows[rowIndex]->value._array->values[elementIndex],
											multiply(arrayRows[colIndex]->value._array->values[elementIndex],tempValue)
										)
									);
									// also for the unity matrix
									outputValue("\t\tTemp value: ",tempValue,"\n");
									assignValue(
										&unityMatrixRows[rowIndex]->value._array->values[elementIndex],
										subtract(
											unityMatrixRows[rowIndex]->value._array->values[elementIndex],
											multiply(unityMatrixRows[colIndex]->value._array->values[elementIndex],tempValue)
										)
									);
									outputValue("\t\tTemp value: ",tempValue,"\n");
								}
								output("\t\tElement loop done!\n");
							}
							output("\t\tRow test done!\n");
						}
						output("\t\tRow loop done!\n");
						outputArray("\t\tRows replaced: ",_arrayCopy,"\n");
						outputArray("\t\t\tUnity matrix: ",unityMatrix,"\n");
					}
					// 3. multiply each row by a nonzero integer
					//    divide row element by the diagonal element
					output("Normalization.\n");
					for(long long rowIndex=0;rowIndex<numberOfArrayRows;rowIndex++){
						output("\tRow=%lld.\n",rowIndex);
						Mvalue* tempValue=arrayRows[rowIndex]->value._array->values[rowIndex];
						if(tempValue==NULL){FREE_ARRAY(unityMatrix,owner);unityMatrix=NULL;break;}
						outputValue("\t\tDivider:",tempValue,"\n");
						for(long long colIndex=0;colIndex<numberOfArrayRows;colIndex++){
							assignValue(
								&arrayRows[rowIndex]->value._array->values[colIndex],
								divide(
									arrayRows[rowIndex]->value._array->values[colIndex],
									tempValue
								)
							);
							assignValue(
								&unityMatrixRows[rowIndex]->value._array->values[colIndex],
								divide(
									unityMatrixRows[rowIndex]->value._array->values[colIndex],
									tempValue
								)
							);
						}
						outputArray("\t\tNormalized: ",_arrayCopy,"\n");
						outputArray("\t\tUnity matrix: ",unityMatrix,"\n");
					}
					/*
					for(colIndex=0;colIndex<numberOfArrayRows;colIndex++){
						// step 1
						// step 2
					}
					if(colIndex<0){FREE_ARRAY(unityMatrix,owner);return NULL;}
					// step 3
					// step 4
					*/
				}else
					outputError("Failed to create a unity matrix");
				FREE_ARRAY(diagonalOfOnes,owner); // not bound in unityMatrix, so requires freeing
			}else
				outputError("Failed to create a unity matrix");
			FREE_ARRAY(_arrayCopy,owner);
			if(unityMatrix!=NULL)return unityMatrix;
			outputError("Failed to invert the matrix");
		}else
			outputError("Failed to copy the matrix to invert");
	}else
		outputError("Can't invert a non-square matrix");
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
	/* MDH@25APR2023: decided NOT to do this and instead create a separate matrix multiplication
	// two-dimensional numeric matrices should be matrix multiplied
	if(_value1->type==VT_ARRAY&&_value2->type==VT_ARRAY)
		if(isANumericMatrix(_value1->value._array)==M_TRUE&&isANumericMatrix(_value2->value._array)==M_TRUE)
			return _getValueOfArray(matrixproduct(_value1->value._array,_value2->value._array));
	*/
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
	/*
	if(_value1->type==VT_ARRAY&&_value2->type==VT_ARRAY)
		if(isANumericMatrix(_value1->value._array)==M_TRUE&&isANumericMatrix(_value2->value._array)==M_TRUE)
			return _getValueOfArray(matrixproduct(_value1->value._array,matrixinverse(_value2->value._array)));
			*/
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

// MDH@12APR2023: you can use Mmatrix() to create a matrix
/**
 * @brief returns a new matrix
 * @details both \p fillValue and \p colsValue can be NULL
 *          if \p fillValue is NULL \p colsValue can be a one-dimensional array
 *          to fill the matrix with
 * @param rowsValue the number of rows the new matrix should have
 * @param colsValue the number of columns the new matrix should have
 * @param fillValue the value to put in every matrix element
 * @return Mvalue* 
 */
Mvalue* Mmatrix(Mvalue* fillValue,Mvalue* rowsValue,Mvalue* colsValue){Mallocationowner owner=getOwner(__LINE__);
	long long numberOfRows=(NULL==rowsValue?0:getValueInteger(rowsValue));
	long long numberOfCols=(NULL==colsValue?0:getValueInteger(colsValue));
	// rowsValue can be NULL or a positive integer
	if(numberOfRows>=0&&numberOfCols>=0){ // acceptable number of rows and columns
		if(fillValue!=NULL){
			// if rowsValue should either be an integer or an array
			if(fillValue->type==VT_ARRAY){
				if(numberOfRows==0){ // no rows specified, so fillValues should be a two-dimensional numeric array at least
					// check if fillValue holds a two-dimensional array with numeric values
					// if it does the two-dimensional array is marked as being a matrix
					// (this will force keeping the matrix a matrix (yet to be implemented though))
					if(fillValue->value._array!=NULL&&isANumericMatrix(fillValue->value._array,true)!=M_TRUE)
						outputError("Argument does not represent an all numeric two-dimensional array");
					// I'm going to be lenient in even returning rowsValue
					return fillValue;
				}else{ // rowsValue indicates the number of rows each to get the values in fillValue
					Marray* _matrix=OWNED_ARRAY(_getArray(NULL,numberOfRows,fillValue),owner);
					if(_matrix!=NULL)return _getValueOfArray(DISOWNED_ARRAY(_matrix,owner));
				}
			}else{ // fillValue is NOT an array!!!!
				if(numberOfRows>0){
					// the columns is also allowed to be an array indicating the value to use as row
					if(numberOfCols<=0){ // not a number
						if(colsValue->type==VT_ARRAY&&colsValue->value._array!=NULL){
							Marray* rowsArray=OWNED_ARRAY(_getArray(NULL,numberOfRows,colsValue),owner);
							if(rowsArray!=NULL){
								if(!isANumericMatrix(rowsArray,true))
									outputError("Two-dimensional array not a (numeric) matrix!");
								return _getValueOfArray(DISOWNED_ARRAY(rowsArray,owner));
							}
						}else
							outputError("No initial row value specified!");
					}else{ // multiple columns
						Mvalue* initialRowValue=_getValueOfArray(_getArray(NULL,numberOfCols,fillValue));
						Marray* _array=OWNED_ARRAY(_getArray(NULL,numberOfRows,initialRowValue),owner);
						if(isNumeric(fillValue))_array->numberOfDimensionsLeft=1; // mark as matrix
						else outputError("The resulting two-dimensional array cannot be used as a matrix");
						return _getValueOfArray(DISOWNED_ARRAY(_array,owner));
					}
				}else
					outputError("Missing number of matrix rows");
			}
		}else
		if(numberOfRows>0&&numberOfCols>0){
			Mvalue* initialRowValue=_getValueOfArray(_getArray(NULL,numberOfCols,NULL));
			Marray* _array=OWNED_ARRAY(_getArray(NULL,numberOfRows,initialRowValue),owner);
			// TODO DONE is a two-dimensional arrays of NULLs numeric????? I guess not!!!
			//////if(isNumeric(fillValue))_array->numberOfDimensionsLeft=1;else // mark as matrix
			outputError("The resulting two-dimensional array with NULL values cannot be used as a matrix");
			return _getValueOfArray(DISOWNED_ARRAY(_array,owner));
		}else
			outputError("Missing number of matrix rows and/or columns");
	}else
		outputError("The matrix rows and/or columns argument do not represent a positive integer");
	return NULL;
}

// MDH@01MAY2023: matrix multiplication separate
/**
 * @brief returns the wrapped matrix product of M matrix \p _value1 and M matrix \p _value2
 * 
 * @param _value1 
 * @param _value2 
 * @return Mvalue* the wrapped matrix product of M matrix \p _value1 and M matrix \p _value2
 */
Mvalue* Mmatrixproduct(Mvalue* _value1,Mvalue* _value2){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value1||NULL==_value2)return NULL;
	if(_value1->type!=VT_ARRAY||_value2->type!=VT_ARRAY)return NULL;
	if(isANumericMatrix(_value1->value._array,true)==M_TRUE||isANumericMatrix(_value2->value._array,true)==M_TRUE){
		Marray* _product=OWNED_ARRAY(matrixproduct(_value1->value._array,_value2->value._array),owner);
		return _getValueOfArray(DISOWNED_ARRAY(_product,owner));
	}
	outputError("Not all arguments of matrix multiplication are numeric matrices");
	return NULL;
}

/**
 * @brief returns the (wrapped) inverse of M matrix \p _value 
 * @details returns NULL if \p value is not invertible
 * @param _value 
 * @return Mvalue* the (wrapped) inverse of M matrix \p _value
 */
Mvalue* Mmatrixinverse(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value||_value->type!=VT_ARRAY)return NULL;
	if(isANumericMatrix(_value->value._array,true)==M_TRUE){
		Marray* _inverse=OWNED_ARRAY(matrixinverse(_value->value._array),owner);
		if(_inverse!=NULL)return _getValueOfArray(DISOWNED_ARRAY(_inverse,owner));
	}else
		outputError("Argument of matrix inverse not a numeric matrix");
	return NULL;
}

/**
 * @brief returns a new matrix with the values in 1-dimensional array \p _value along the diagonal
 * @details returns NULL if \p _value is NULL or not containing an array
 * @param _value 
 * @return Mvalue* a new matrix with the values in 1-dimensional array \p _value along the diagonal
 */
Mvalue* Mmatrixdiagonal(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value)return NULL;
	if(_value->type==VT_ARRAY){
		Marray* _diagonalmatrix=OWNED_ARRAY(diagonalmatrix(_value->value._array->values,_value->value._array->numberOfElements),owner);
		return _getValueOfArray(DISOWNED_ARRAY(_diagonalmatrix,owner));
	}
	outputError("Argument to the diagonal matrix function not an array");
	return NULL;
}	

/**
 * @brief returns the transpose of square matrix \p _value
 * @returns NULL if \p _value does not contain a square matrix
 * @param _value 
 * @return Mvalue* the transpose of square matrix \p _value
 */
Mvalue* Mmatrixtranspose(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value)return NULL;
	if(_value->type==VT_ARRAY){
		// does being a matrix suffice????
		Marray* array=_value->value._array;
		if(isAMatrix(array)){
			long long numberOfRows=getNumberOfMatrixRows(array);
			if(numberOfRows==getNumberOfMatrixColumns(array)){
				// create a copy of array
				Marray* _array=OWNED_ARRAY(_getArrayCopy(array),owner);
				if(NULL==_array){outputError("Failed to copy the matrix to transpose");return NULL;}
				// transpose _array
				// NOTE we're simply exchanging the pointers (and not using assignValue!!!)
				Mvalue** arrayRows=_array->values;
				for(long long rowIndex=0;rowIndex<numberOfRows;rowIndex++){
					Mvalue** arrayRowValues=arrayRows[rowIndex]->value._array->values; // the values in the row #rowIndex
					for(long long colIndex=rowIndex+1;colIndex<numberOfRows;colIndex++){
						Mvalue* tempValue=arrayRowValues[colIndex];
						arrayRowValues[colIndex]=arrayRows[colIndex]->value._array->values[rowIndex];
						arrayRows[colIndex]->value._array->values[rowIndex]=tempValue;
					}
				}
				return _getValueOfArray(DISOWNED_ARRAY(_array,owner));
			}else
				outputError("Argument to the transpose function not a square matrix");
		}else
			outputError("Argument to the transpose function not a matrix");
	}else
		outputError("Argument to the matrix transpose function not an array");
	return NULL;
}	

/**
 * @brief returns the trace of square matrix \p _value
 * 
 * @param _value 
 * @return Mvalue* the trace of square matrix \p _value
 */
Mvalue* Mmatrixtrace(Mvalue* _value){
	if(NULL==_value)return NULL;
	if(_value->type==VT_ARRAY){
		// does being a matrix suffice????
		Marray* array=_value->value._array;
		if(isANumericMatrix(array,true)){
			long long numberOfRows=getNumberOfMatrixRows(array);
			if(numberOfRows==getNumberOfMatrixColumns(array)){
				Mvalue** arrayRows=array->values;
				Mvalue* traceValue=getValueZeroOfType(getMatrixElementValuetype(array));
				for(long long rowIndex=0;rowIndex<numberOfRows;rowIndex++)
					traceValue=add(traceValue,arrayRows[rowIndex]->value._array->values[rowIndex]);
				return traceValue;
			}else
				outputError("Argument to the transpose function not a square matrix");
		}else
			outputError("Argument to the transpose function not a matrix");
	}else
		outputError("Argument to the matrix transpose function not an array");
	return NULL;
}


/**
 * @brief returns the determinant of square matrix \p _value
 * @details returns NULL if \p _value is not square
 *          uses Heap's algorithm to generate all possible product permutations
 * @param _value 
 * @return Mvalue* the determinant of square matrix \p _value
 */
Mvalue* Mmatrixdeterminant(Mvalue* _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value)return NULL;
	if(_value->type==VT_ARRAY){
		// does being a matrix suffice????
		Marray* array=_value->value._array;
		if(isANumericMatrix(array,true)){
			long long numberOfRows=getNumberOfMatrixRows(array);
			if(numberOfRows==getNumberOfMatrixColumns(array)){
				Mvalue** arrayRows=array->values;
				Mvalue* determinantValue=NULL;
				// we can use the first column and therefore exclude rows
				// we know how many to exclude in total that is one less than the total number of rows
				long long * permutation=(long long *)CALLOC(sizeof(long long),numberOfRows,-'x',owner);
				long long * c=(long long *)CALLOC(sizeof(long long),numberOfRows,-'x',owner);
				// keeping track of the cumulative product to add or subtract to the 
				Mvalue* *cumproduct=(Mvalue**)CALLOC(sizeof(Mvalue*),numberOfRows,'X',owner);
				if(c!=NULL&&permutation!=NULL&&cumproduct!=NULL){
					// initialize the cumulative product to the cumulative product of the diagonal elements
					assignValue(cumproduct,arrayRows[0]->value._array->values[0]);
					for(long long rowIndex=1;rowIndex<numberOfRows;rowIndex++)
						assignValue(cumproduct+rowIndex,
							multiply(cumproduct[rowIndex-1],arrayRows[rowIndex]->value._array->values[rowIndex]));
					assignValue(&determinantValue,cumproduct[numberOfRows-1]);
					/////////outputValue("Initial value determinant: ",determinantValue,"\n");
					if(determinantValue!=NULL){
						//if(amVerbose())
							outputValue("Determining the determinant of ",_value,".\n");
						// initialize permutation to the possible row indices
						long long rowIndex=numberOfRows;while(--rowIndex>=0)permutation[rowIndex]=rowIndex;
						bool neg=false;
						// permutate permutation
						long long temp,swapi,i=1,count=0;
						while(1){ // replacing: i<numberOfRows
							if(i>=numberOfRows||c[i]<i){
								//if(amVerbose()){
									output("\tUpdated determinant after %s product #%lld",(neg?"subtracting":"adding"),++count);
									outputValue(" (",cumproduct[numberOfRows-1],") of cells");
									for(long long permIndex=0;permIndex<numberOfRows;permIndex++)
										output(" (%lld,%lld)",permutation[permIndex],permIndex);
									outputValue(": ",determinantValue,"\n");
								//}
								if(i>=numberOfRows)break;
								swapi=(i%2?c[i]:0);
								temp=permutation[swapi];
								permutation[swapi]=permutation[i];
								permutation[i]=temp;
								// use the ninimum of swapi and i to determine the first cum product to update
								if(swapi>i)swapi=i;
								// update all cumulative products starting at swapi
								if(swapi==0){
									assignValue(cumproduct,arrayRows[permutation[swapi]]->value._array->values[0]);
									swapi++;
								}
								for(;swapi<numberOfRows;swapi++)
									assignValue(cumproduct+swapi,
										multiply(cumproduct[swapi-1],arrayRows[permutation[swapi]]->value._array->values[swapi]));
								//outputValue("Cum product: ",cumproduct[numberOfRows-1],"\n");
								// increment the determinant with the new cumulative product
								if(neg){ // toggle to false
									neg=false;
									assignValue(&determinantValue,add(determinantValue,cumproduct[numberOfRows-1]));
								}else{ // toggle to true
									neg=true;
									assignValue(&determinantValue,subtract(determinantValue,cumproduct[numberOfRows-1]));
								}
								c[i]++;
								i=1;
							}else
								c[i++]=0;
						}
						// initialize determinantValue to the cumulative product of the diagonal elements
						// NOTE c is already initialized to zeroes as we need them in Heap's algorithm
						/* here's a Python implementation of Heap's algorithm (see heapsalgorithm.py)
						def generate(n,A):
							def swap(i,j):
								temp=A[i]
								A[i]=A[j]
								A[j]=temp
							c=[0]*n
							i=1
							count=1
							print("1: ",A)
							while i<n:
								if c[i]<i:
									if i%2:
										swap(0,i)
									else:
										swap(c[i],i)
									count+=1
									print(count,": ",A)
									c[i]+=1
									i=1
								else:
									c[i]=0
									i+=1
						*/
						// using Heap's algorithm to generate all permutations of the row indices of each cell to use
					}else
						outputError("Failed to initialize the determinant");
				}
				if(amVerbose())
					outputValue("Determinant: ",determinantValue,"\n");
				FREE_DISOWNED(permutation,numberOfRows,-'x',owner);
				FREE_DISOWNED(c,numberOfRows,-'x',owner);
				FREE_DISOWNED(cumproduct,numberOfRows,'X',owner);
				return determinantValue;
			}else
				outputError("Argument to the transpose function not a square matrix");
		}else
			outputError("Argument to the transpose function not a matrix");
	}else
		outputError("Argument to the matrix transpose function not an array");
	return NULL;
}
#include "Moperations.h"
/*
Miterator* getMatrixIterator(Mmatrix* _matrix); 
*/

long long getNumberOfMatrixRows(Marray* array);
long long getNumberOfMatrixColumns(Marray* array);
long long isAMatrix(Marray* array);
long long isANumericMatrix(Marray* array);

Marray* matrixproduct(Marray* array1,Marray* array2);
Marray* matrixinverse(Marray* array1);

Mvalue* multiply(Mvalue* _value1,Mvalue* _value2);
Mvalue* divide(Mvalue* _value1,Mvalue* _value2);

// if you want to create a matrix or turn a numeric two-dimensional array into a matrix 
// apply the matrix() function
Mvalue* Mmatrix(Mvalue* rowsValue,Mvalue* colsValue,Mvalue* fillValue);

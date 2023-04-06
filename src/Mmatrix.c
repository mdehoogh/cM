// MDH@04APR2023: matrices are two-dimensional arrays with data in an associated Marray

#include "Mmatrix.h"

extern unsigned long long M_MODULE_DEBUGGING;

// MDH@22NOV2020
static Mallocationowner getOwner(uint16_t id){return (Mallocationowner){MI_MATRIX,id};}

Miterator** getMatrixIterators(Mmatrix* matrix){
	return NULL;
}
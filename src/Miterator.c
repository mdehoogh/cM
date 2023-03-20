#include "Miterator.h"

extern const unsigned long long M_MODULE_DEBUGGING;

// some convenience methods that do not require calling next yourself in essence hiding the Msequenceelement structure
// however, it is a nuisance that next should also return index?????? NO, this means you can use iterator->next() only
/**
 * @brief returns the next iterator value
 * 
 * @param iterator 
 * @return Mvalue* the next iterator value
 */
Mvalue* iter_next(Miterator const * const iterator){
	return(iterator!=NULL?iterator->next(iterator).value:NULL);
}
/**
 * @brief returns the (1-based) next iterator index of \p iterator
 * 
 * @param iterator 
 * @return unsigned long long the positive next iterator index, or 0 if \p iterator is NULL
 */
unsigned long long iter_nextindex(Miterator const * const iterator){
	return(iterator!=NULL?iterator->nextindex(iterator):0);
}
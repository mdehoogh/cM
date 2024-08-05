// some common list functions
#include "Marray.h"

// (sequence) iterator support
Miterator getListiterator(Mlist* list);

Mvalue* Mcreatelist(Mvalue* lengthValue,Mvalue* fillValue);
Mvalue* Mclear(Mvalue* value); // will also work on a map though (TODO move over to functions.h/c I guess)
Mvalue* Mempty(Mvalue* value); // will also work on a map though (TODO move over to functions.h/c I guess)
Mvalue* Mkeys(Mvalue* value); // will also work on a map (although perhaps those are attributes)

Mvalue* Mpush(Mvalue* listValue,Mvalue* value); // append a value to the list (also referred to as drop)
Mvalue* Mshove(Mvalue* listValue,Mvalue* value); // prepend a value to a list
Mvalue* Minsert(Mvalue* listValue,Mvalue* value,Mvalue* indexValue); // prepend a value to a list

Mvalue* Mpop(Mvalue* listValue); // remove and return the last value (how about takeoff)
Mvalue* Mpull(Mvalue* listValue); // remove and return the first value
Mvalue* Mremoved(Mvalue* listValue,Mvalue* listIndexValue); // remove a certain list element (given the index of the list element)
Mvalue* Mfind(Mvalue* listValue,Mvalue* listElementValue,Mvalue* maximumNumberOfElementsToFindValue); // returns the indices of the elements in listValue that match listElementValue

Mvalue* Mfirst(Mvalue* listValue); // return the first value
Mvalue* Mlast(Mvalue* listValue); // return the last value

// value type specific sample statistics methods (TODO we could move these to the iterator module)
Mmap* _getIntegerSampleStatisticsMap(Miterator* iterator);
Mmap* _getBigintegerSampleStatisticsMap(Miterator* iterator);
Mmap* _getFloatSampleStatisticsMap(Miterator* iterator);
Mmap* _getDecimalSampleStatisticsMap(Miterator* iterator);
Mmap* _getRationalSampleStatisticsMap(Miterator* iterator);

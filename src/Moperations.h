#include "Mlist.h"

// helper functions
/*
Marray* _appliedToArrayElements(Marray* _array,OneArgumentFunction oneArgumentFunction,bool maintainsValuetype);
Mlist* _appliedToListElements(Mlist* _list,OneArgumentFunction oneArgumentFunction,bool maintainsValuetype);
Mmap* _appliedToMapElements(Mmap* _map,OneArgumentFunction oneArgumentFunction,bool maintainsValuetype);
*/
Mlist* _appliedToLists(Mlist* _list1,Mlist* _list2,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Mlist* _appliedToListAndArray(Mlist* _list,Marray* _array,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Marray* _appliedToArrayAndList(Marray* _array,Mlist* _list,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Mvalue* _appliedToList(Mlist* _list,Mvalue* _value,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Mvalue* _appliedToList2(Mvalue* _value,Mlist* _list,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Marray* _appliedToArrays(Marray* _array1,Marray* _array2,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Mvalue* _appliedToArray(Marray* _array,Mvalue* _value,TwoArgumentFunction binaryoperator,bool maintainsValuetype);
Mvalue* _appliedToArray2(Mvalue* _value,Marray* _array,TwoArgumentFunction binaryoperator,bool maintainsValuetype);

Mvalue* Mneg(Mvalue* value);

// binary operator functions
Mvalue* add(Mvalue* value1,Mvalue* value2);
//////Mvalue* multiply(Mvalue* value1,Mvalue* value2);
Mvalue* subtract(Mvalue* value1,Mvalue* value2);
//////Mvalue* divide(Mvalue* value1,Mvalue* value2);
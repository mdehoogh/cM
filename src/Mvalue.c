#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <dirent.h>
#include <locale.h>
#include <errno.h>
#include <ctype.h>

#include "Mvalue.h"

// in order to be able to use access to determine if a file exists
#if defined _WIN32 || defined _WIN64 || defined __WIN32 || defined _WCE || defined MSDOS || defined __MSDOS || defined OS2 || defined _OS2 || defined __OS2___
#include <io.h>
#define F_OK 0
#define access _access
#define ftello _ftelli64
#define fseeko _fseeki64
#else
#include <unistd.h>
#include <sys/types.h>
// force off_t to be 64 bits because we want to be able to access files larger than 2GB
#define _FILE_OFFSET_BITS 64
#endif
#include <time.h>

static bool DEBUGGING=true;

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(int16_t id){return(Mallocationowner){MI_VALUE,id};}
#define OWNER(id) {MI_VALUE,(id)}

extern const long long M_LL_INVALID,M_LL_MIN,M_LL_MAX,M_TRUE,M_FALSE,M_POSITIVE,M_NEGATIVE,M_ZERO,M_ARRAY_ELEMENTS_AT_START,M_ARRAY_ELEMENTS_AT_END,M_LIST_ELEMENTS_AT_START,M_LIST_ELEMENTS_AT_END;
extern const char * const VALUETYPENAMES[]; // the characters associated with each of the value types
extern const char * const MUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const IMMUTABLEVALUETYPECHARS; // the characters associated with each of the value types
extern const char * const M_ERROR_PREFIX;
extern const char * const M_WARNING_PREFIX;
extern const char * const M_BUG_PREFIX;
extern const char * const M_NULL_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value that is NULL
extern const char * const M_UNDEFINED_VALUE_TEXT; // MDH@31OCT2019: the text to use to represent a value of type VT_UNDEFINED
extern const long double M_LD_Q_EPS; // the threshold for accepting a rational approximation of a long double
extern const long double M_LD_NAN; // we'll be needing this in Mexecution.c as well but M.c sets it!!
extern const long double LD_PI; // for Mfacd()
extern Mdecimalcontext * const M_DECIMALCONTEXT; // the application-wide (default) decimal context
extern const char M_DEREFERENCE_CHARACTER; // MDH@11MAR2020

 // MDH@10JAN2020
 /**
  * @brief return M_TRUE if \p _variable is immutable, M_FALSE otherwise, but M_LL_INVALID if _variable is NULL
  * 
  * @param _variable 
  * @return long long M_TRUE, M_FALSE or M_LL_INVALID
  */
long long isImmutable(Mvariable* _variable){
	return(_variable!=NULL?(_variable->unlockCode>0?M_TRUE:M_FALSE):M_LL_INVALID);
}
/**
 * @brief sets the immutable flag of \p _variable to \p immutable
 * 
 * @param _variable 
 * @param immutable 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long setImmutable(Mvariable* _variable,long long unlockCode){
	if(NULL==_variable)return M_LL_INVALID;
	long long result=(_variable->unlockCode>0?M_TRUE:M_FALSE);
	_variable->unlockCode=unlockCode;
	return result;
}

/**
 * @brief returns \p _variable disowned by \p owner_variable
 * 
 * @param _variable 
 * @param owner_variable 
 * @return Mvariable* \p _variable disowned by \p owner_variable
 */
Mvariable* disowned_variable(Mvariable* _variable,Mallocationowner owner_variable){
	if(_variable->_name!=NULL){
		////////output("Disowning variable '%s'.\n",_variable->_name); // DEBUG
		disowned_chars(_variable->_name,owner_variable);
		// output("Variable '%s' disowned.\n",_variable->_name); // DEBUG
	} // dynamically allocated (indicated by _) so we should free it...
	return(Mvariable*)DISOWNED(_variable,owner_variable);
}
/**
 * @brief returns \p _variable owned by \p owner_variable
 * 
 * @param _variable 
 * @param owner_variable 
 * @return Mvariable* \p _variable owned by \p owner_variable
 */
Mvariable* owned_variable(Mvariable* _variable,Mallocationowner owner_variable){
	if(_variable->_name)owned_chars(_variable->_name,Msubowner(owner_variable,1)); // dynamically allocated (indicated by _) so we should free it...
	return(Mvariable*)OWNED(_variable,owner_variable);
}
/**
 * @brief frees \p _variable
 * @details when flag \p weak is not set, the reference count of the value of \p variable is decremented
 * @param _variable 
 * @param weak 
 */
void free_variable(Mvariable* _variable,bool weak){
	if(_variable==NULL)return;
	if(_variable->_name!=NULL)freeChars(_variable->_name); // dynamically allocated (indicated by _) so we should free it...
	if(!weak)if(_variable->_value!=NULL)decrementReferenceCount(_variable->_value); //// replacing: free_value(_variable->_value);
	FREE_1(_variable,'V');
}/* VALIDATED */

// MDH@09JUN2020: if we want the name of the variable to be subowned by the caller, it's better to pass in a properly owned Mchars name, which we can subown
//				it's a bit of a nuisance that we create it in such a way that the caller has to take care of the ownership of _name but that makes sense because we pass in Mchars (which is already managed)
// MDH@11JUN2020: by allowing _name to be disowned to start with we can free it when we fail to bind it
/**
 * @brief returns an M variable with name \p name of value type \p valuetype with unlock code equal to \p unlockCode
 * 
 * @param _name 
 * @param valuetype 
 * @param unlockCode 
 * @return Mvariable* an M variable with name \p name of value type \p valuetype with unlock code equal to \p unlockCode
 */
Mvariable* _getVariable(Mchars const * const _name,Mvaluetype valuetype,long long unlockCode){Mallocationowner owner=getOwner(__LINE__);
	// MDH@14NOV2019: maps might have attributes with no name (i.e. the empty string)
	if(_name!=NULL){
		Mvariable* _variable=(Mvariable*)CALLOC_1(sizeof(Mvariable),'V',owner); // all pointers will be NULL!!
		if(_variable!=NULL){
			_variable->_name=(Misdisowned(_name)?owned_chars(_name,Msubowner(owner,1)):_name); // MDH@17APR2020 _strdup() replaced by _getChars(): // create a dynamic pointer on the heap
			_variable->unlockCode=unlockCode;
			_variable->valuetype=valuetype;
			// output("Returning new disowned variable '%s'.\n",_name); // DEBUG
			return disowned_variable(_variable,owner);
		}
		if(Misdisowned(_name))freeChars(_name);
		outputError("Failed to create a variable.");
	}else
		outputError("No variable name defined");
	/*
	 if(!_variable->_name){
		freeChars(_name,owner);
		free_variable(_variable,true,owner);
		output("%sFailed to allocate memory to store name '%s' of the new variable.\n",M_ERROR_PREFIX,_name->chars);
		return NULL;
	}
	_variable->valuetype=valuetype;
	*/
	/* MDH@01MAY2019: we're NOT setting the value here!!!!
	// a ha, here we have an issue: we cannot just use the char pointer, if we want to free the name later on
	if(_variable->valuetype!=VT_UNDEFINED){
		_variable->_value=(Mvalue*)calloc(1,sizeof(Mvalue));
		if(!_variable->_value){
			free_variable(_variable);
			printf("\nERROR: Failed to allocate memory to store the value of variable '%s'.",name);
			return NULL;
		}
		_variable->_value->type=valueType;
		// I do NOT need to set the values of the atomic elements (integer, real), so basically these are not initialized to start with
		// NOTE using calloc() instead of malloc() is essential for everything with pointers in it that should be NULL initially!!
		switch(_variable->valueType){
			case VT_UNDEFINED:
			case VT_INTEGER:
			case VT_FLOAT:break;
			case VT_TEXT:_variable->_value->value._text=(Mtext*)calloc(1,sizeof(Mtext));break;
			case VT_LIST:_variable->_value->value._list=(Mlist*)calloc(1,sizeof(Mlist));break;
			case VT_MAP:_variable->_value->value._map=(Mmap*)calloc(1,sizeof(Mmap));break;
		}
	}else
		_variable->_value=NULL;
	*/
	// MDH@04JUN2020: given that _variable is already disowned (as returned by CALLOC_1), no need to disown it again
	return NULL; // replacing: return DISOWNED(_variable,owner);
}/* VALIDATED */
/**
 * @brief returns \p _listelement owned by \p owner_listelement
 * 
 * @param _listelement 
 * @param owner_listelement 
 * @return Mlistelement* \p _listelement owned by \p owner_listelement
 */
Mlistelement* owned_listelement(Mlistelement* _listelement,Mallocationowner owner_listelement){
	if(NULL==_listelement)return NULL;
	if(_listelement->_next!=NULL)owned_listelement(_listelement->_next,owner_listelement);
	return OWNED(_listelement,owner_listelement);
}
/**
 * @brief returns \p _listelement disowned by \p owner_listelement
 * 
 * @param _listelement 
 * @param owner_listelement 
 * @return Mlistelement* \p _listelement disowned by \p owner_listelement
 */
Mlistelement* disowned_listelement(Mlistelement* _listelement,Mallocationowner owner_listelement){
	if(NULL==_listelement)return NULL;
	if(_listelement->_next!=NULL)disowned_listelement(_listelement->_next,owner_listelement);
	return DISOWNED(_listelement,owner_listelement);
}
// MDH@18JUN2020: how about returning the number of list elements removed or perhaps the last list element removed????
//				let's return the number of list elements freed
/**
 * @brief frees M list element \p _listelement (and all the list elements it points to)
 * @details if flag \p weak is not set, the reference count of the value associated with the list element is decremented
 * @param _listelement
 * @returns M_LL_INVALID if \p _listelement is not defined, otherwise the number of list elements freed
 */
long long free_listelement(Mlistelement* _listelement,bool weak/*,Mallocationowner owner*/){
	long long result=M_LL_INVALID; // when there's nothing to free!!!!
	if(_listelement!=NULL){
		// free_listelement on a non-null next will ALWAYS return a positive value
		result=(_listelement->_next!=NULL?free_listelement(_listelement->_next,weak):0);_listelement->_next=NULL;
		if(_listelement->_value!=NULL){if(!weak)decrementReferenceCount(_listelement->_value);_listelement->_value=NULL;} ///////// replacing: free_value(_listelement->_value);
		FREE_1(_listelement,'l'/*,owner*/);
		result+=1;
	}
	return result;
}/* VALIDATED */
/**
 * @brief returns M list \p _list owned by \p owner_list
 * 
 * @param _list 
 * @param owner_list 
 * @return Mlist* M list \p _list owned by \p owner_list
 */
Mlist* owned_list(Mlist* _list,Mallocationowner owner_list){
	if(!_list)return NULL;
	if(_list->_creator)owned_chars(_list->_creator,Msubowner(owner_list,1));
	if(_list->_first)owned_listelement(_list->_first,owner_list);
	return OWNED(_list,owner_list);
}
/**
 * @brief returns M list \p _list disowned by \p owner_list
 * 
 * @param _list 
 * @param owner_list 
 * @return Mlist* M list \p _list disowned by \p owner_list
 */
Mlist* disowned_list(Mlist* _list,Mallocationowner owner_list){
	if(!_list)return NULL;
	if(_list->_creator)disowned_chars(_list->_creator,owner_list);
	if(_list->_first)disowned_listelement(_list->_first,owner_list);
	return DISOWNED(_list,owner_list);
}
/**
 * @brief frees M list \p _list
 * 
 * @param _list 
 */
void free_list(Mlist* _list){
	if(amVerboseDebugging())
	{output("Freeing a %s list",_list->weak?"weak":"strong");if(_list->_creator)output(" created by '%s'",_list->_creator->chars);outputChar('.');outputChar('\n');}
	if(_list->_first){
		// MDH@17APR2020: assuming we allocated exactly the number of characters for storing the characters
		if(_list->_creator)freeChars(_list->_creator/*,owner*/);// MDH@17APR2020 replacing: FREE_DISOWNED_1(_list->_creator,'"');
		free_listelement(_list->_first,_list->weak/*,owner*/);
		_list->_first=NULL;
	}
	FREE_1(_list,'L'/*,owner*/);
}/* VALIDATED */
/**
 * @brief returns a new M list
 * @param source a text describing the requestee that is stored as creator of the M list
 * @returns a new M list
 */
Mlist* __list(char* source/*,Mallocationowner owner_list*/){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=CALLOC_1(sizeof(Mlist),'L',owner);
	if(source){
		_list->_creator=owned_chars(_getChars(source),Msubowner(owner,1)); // replacing: keep disowned, so easy to reown... SUBOWNED(OWNED(_getChars(source),owner),1); // MDH@17APR2020 replacing: _strdup(source);
		if(!_list->_creator)
			output("%sFailed to register list creator '%s'.\n",M_ERROR_PREFIX,source);
		else
		if(amVerboseDebugging())
			output("List creator: '%s'.\n",_list->_creator->chars);
	}
	return DISOWNED_LIST(_list,owner);
}

// MDH@04NOV2020: coming soon in this theater
#ifndef __PRODUCTION__
/**
 * @brief returns \p _mapelement owned by \p owner_mapelement
 * 
 * @param _mapelement 
 * @param owner_mapelement 
 * @return Mmapelement* returns \p _mapelement owned by \p owner_mapelement
 */
Mmapelement* owned_mapelement(Mmapelement* _mapelement,Mallocationowner owner_mapelement){
	if(NULL==_mapelement)return NULL;
	if(_mapelement->_next!=NULL)owned_mapelement(_mapelement->_next,owner_mapelement);
	if(_mapelement->_variable!=NULL)owned_variable(_mapelement->_variable,Msubowner(owner_mapelement,1));
	//DEBUGGINGoutput("Owning a map element!\n");
	return OWNED(_mapelement,owner_mapelement);
}
/**
 * @brief returns \p _mapelement disowned by \p owner_mapelement
 * 
 * @param _mapelement 
 * @param owner_mapelement 
 * @return Mmapelement* returns \p _mapelement disowned by \p owner_mapelement
 */
Mmapelement* disowned_mapelement(Mmapelement* _mapelement,Mallocationowner owner_mapelement){
	if(NULL==_mapelement)return NULL;
	//DEBUGGINGoutput("Disowning a map element!\n");
	if(_mapelement->_variable!=NULL){
		//DEBUGGINGoutput("Disowning map element variable '%s'.\n",_mapelement->_variable->_name,".\n");
		disowned_variable(_mapelement->_variable,owner_mapelement);
	}
	if(_mapelement->_next!=NULL)disowned_mapelement(_mapelement->_next,owner_mapelement);
	return DISOWNED(_mapelement,owner_mapelement);
}
#endif
/**
 * @brief returns a newly allocated map element
 * 
 * @param source 
 * @return Mmapelement* a newly allocated map element
 */
Mmapelement* __mapelement(char const * const source){Mallocationowner owner=getOwner(__LINE__);
	Mmapelement* _mapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner);
	output("Map element created for source '%s'.\n",source);
	return(NULL==_mapelement?NULL:disowned_mapelement(_mapelement,owner));
}

/**
 * @brief frees map element \p _mapelement
 * @details if \p weak is true, \p _mapelement  
 * @returns the number of freed map elements, M_LL_INVALID if _mapelement is NULL
 */
long long free_mapelement(Mmapelement* _mapelement,bool weak/*,Mallocationowner owner*/){
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_VALUE));
	long long result=M_LL_INVALID;
	if(_mapelement!=NULL){
		if(report)
			output("About to free a %s map attribute!\n",(weak?"weak":"strong"));
		if(_mapelement->_next!=NULL){
			result=free_mapelement(_mapelement->_next,weak);
			_mapelement->_next=NULL;
		}else
			result=0;
		if(_mapelement->_variable!=NULL){
			if(_mapelement->_variable->_name!=NULL){
				if(report)
					output("About to free %s map attribute '%s'.\n",(weak?"weak":"strong"),_mapelement->_variable->_name);
			}else
				outputWarning("Unnamed map attribute!");
			free_variable(_mapelement->_variable,weak);
			_mapelement->_variable=NULL; // MDH@11NOV2019: for safety purposes (won't wanna try it again)
		}else
			outputWarning("No map attribute to free!");
		FREE_1(_mapelement,'m'/*,owner*/);
		result+=1;
		if(report)
			outputInfo("\tMap element freed!");
	}
	return result;
}/* VALIDATED */

#ifndef __PRODUCTION__
/**
 * @brief returns \p _map owned by \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @return Mmap* \p _map owned by \p owner_map
 */
Mmap* owned_map(Mmap* _map,Mallocationowner owner_map){
	if(NULL==_map)return NULL; // MDH@22OCT2020: this was missing before, which was causing crashes
	if(_map->_creator!=NULL)owned_chars(_map->_creator,Msubowner(owner_map,1)); // MDH@25JAN2023 BUG FIX: disown the creator owner as well!!!!
	if(_map->_first!=NULL)owned_mapelement(_map->_first,Msubowner(owner_map,1));
	return OWNED(_map,owner_map);
}
/**
 * @brief returns \p _map disowned by \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @return Mmap* \p _map disowned by \p owner_map
 */
Mmap* disowned_map(Mmap* _map,Mallocationowner owner_map){
	if(NULL==_map)return NULL;
  //DEBUGGINGoutput("Disowning map!\n");
	if(_map->_creator!=NULL){
		////////output("Creator: '%s'.\n",_map->_creator);
		disowned_chars(_map->_creator,owner_map);
	} // MDH@25JAN2023 BUG FIX: disown the creator owner as well!!!!
	if(_map->_first!=NULL)disowned_mapelement(_map->_first,owner_map);
	//DEBUGGINGoutput("Map disowned!\n");
	return DISOWNED(_map,owner_map);
}
#endif
// MDH@01NOV2020: why wasn't this here before?
/**
 * @brief returns a new M map
 * 
 * @param source the id of the requestee stored in the creator field of the new map
 * @return Mmap* 
 */
Mmap* __map(char const * const source){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
	if(NULL==_map)return NULL;
	if(source!=NULL){
		///////output("Creating a map of source '%s'.\n",source);
		_map->_creator=owned_chars(_getChars(source),Msubowner(owner,1)); 
		if(NULL==_map->_creator)
			output("%sFailed to register map creator '%s'.\n",M_ERROR_PREFIX,source);
		else
		if(amVerboseDebugging())
			output("Map creator: '%s'.\n",_map->_creator->chars);
	}
	return disowned_map(_map,owner);
}
/**
 * @brief frees M map \p _map
 * @param _map
 */
void free_map(Mmap* _map/*,Mallocationowner owner*/){
	if(NULL==_map)return;
	if(amVerboseDebugging())
		output("About to free a (%s) map with %llu attributes!\n",(_map->weak?"weak":"strong"),_map->numberOfElements);
	if(_map->_first!=NULL){
		free_mapelement(_map->_first,_map->weak);
		_map->_first=NULL;
	}else
	if(amVerboseDebugging()) 
		outputInfo("No map attributes to free!");
	FREE_1(_map,'M'/*,owner*/);
}/* VALIDATED */

// MDH@26OCT2019: when freeing a value reference we NULL the fields just in case (TODO why?)
#ifndef __PRODUCTION__
/**
 * @brief returns \p _valuereference owned by \p owner_valuereference
 * 
 * @param _valuereference 
 * @param owner_valuereference 
 * @return Mvaluereference* \p _valuereference owned by \p owner_valuereference
 */
Mvaluereference* disowned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference){
	if(NULL==_valuereference){outputError("No value reference to disown!");return NULL;}
		//output("%s disowning a value reference!",getAllocationOwnerText(owner_valuereference));
		////output("Disowning value reference '%s'!\n",_valuereference->_name->chars);
	disowned_chars(_valuereference->_name,owner_valuereference);
	return DISOWNED(_valuereference,owner_valuereference);
}
/**
 * @brief returns \p _valuereference disowned by \p owner_valuereference
 * 
 * @param _valuereference 
 * @param owner_valuereference 
 * @return Mvaluereference* \p _valuereference disowned by \p owner_valuereference
 */
Mvaluereference* owned_valuereference(Mvaluereference* _valuereference,Mallocationowner owner_valuereference){
	if(NULL==_valuereference)return NULL;
		////output("Owning value reference '%s'!\n",_valuereference->_name->chars);
	owned_chars(_valuereference->_name,owner_valuereference);
	return OWNED(_valuereference,owner_valuereference);
}
#endif
/**
 * @brief frees M value reference \p _valuereference
 * 
 */
void free_valuereference(Mvaluereference* _valuereference/*,Mallocationowner owner*/){
	if(NULL==_valuereference)return;
	if(_valuereference->_name!=NULL){freeChars(_valuereference->_name/*,owner*/);_valuereference->_name=NULL;}
	/* MDH@02NOV2019: all values now 'weak' assigned i.e. no need to dereference anymore
	if(_valuereference->_value){assignValue(&_valuereference->_value,NULL);_valuereference->_value=NULL;} // get rid of the reference
	*/
	if(_valuereference->_itemid!=NULL){assignValue(&_valuereference->_itemid,NULL);_valuereference->_itemid=NULL;}
	FREE_1(_valuereference,'5'/*,owner*/); // MDH@19NOV2019: type changed from @ to 5 (See M.c for the allocations)
}/* VALIDATED */
// #define FREE_VALUEREFERENCE(_valuereference,owner_valuereference) free_reference(disowned_valuereference(_valuereference,owner_valuereference))

// manage a list of created values
// if we make a map out of it, we can annote the value with a name????
/**
 * @brief the global M value list where all M values are stored susceptible to being garbage collected
 * 
 */
static Mlist* _valueList=NULL;
/**
 * @brief the total number of values in the global M value list
 * 
 */
static unsigned long long valueCount=0; // MDH@16JUN2020: keeping track of the total number of values
// MDH@28MAY2020: in one go we can set the owner of the value list, the owner of every element in the value list, the owner of each value in every element of the value list and finally that of any data bound to the value
static Mallocationowner owner_valueList=(Mallocationowner){MI_VALUE,__LINE__,1};
static Mallocationowner owner_valueListelement=(Mallocationowner){MI_VALUE,__LINE__-1,1,1};
static Mallocationowner owner_value=(Mallocationowner){MI_VALUE,__LINE__-2,1,2};
static Mallocationowner owner_value_data=(Mallocationowner){MI_VALUE,__LINE__-3,1,3};
// MDH@28MAY2020: if someone want to add something to a value (s)he should use getValueOwner() to retrieve the owner of the value
/**
 * @brief returns the global M value owner
 * 
 * @return Mallocationowner the global M value owner
 */
Mallocationowner getValueOwner(){return owner_value;}
/**
 * @brief returns the global M value data owner
 * 
 * @return Mallocationowner the global M value data owner
 */
Mallocationowner getValueDataOwner(){return owner_value_data;}
/**
 * @brief returns a new M value appended to the global M value list
 * 
 * @param descriptor the id of the requestee
 * @return Mvalue* 
 */
Mvalue* __value(char const * const descriptor){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* _value=NULL;
	if(NULL==_valueList){
		_valueList=owned_list(__list("global value list"),owner_valueList); // MDH@19MAY2020: _valueList is global and we use 0 as function owner id (which is the rule for module global variables)
		if(NULL==_valueList){outputBug("Failed to create the global value list.");return NULL;}
		// MDH@28MAY2020: it doesn't really matter what owner we pass to CALLOC_1 because appendedToList() will reposses it
		//				as an alternative we could call __value now to add an empty value representing undefined except that in that case it would get X as value type not U
		Mvalue* _undefinedValue=CALLOC_1(sizeof(Mvalue),'U',owner);
		long long valueIndex=appendedToList(_valueList,owner_valueList,_undefinedValue,M_LL_INVALID);
		if(valueIndex<=0){
			FREE_DISOWNED_1(_undefinedValue,'U',owner);
			outputBug("Failed to store the global undefined value.");
			return NULL;
		}
		OWNED(DISOWNED(_undefinedValue,owner),owner_value); // MDH@06APR2023: take over ownership of the appended value
		//////output("Undefined value index #%lld.\n",valueIndex);
		valueCount=valueIndex;
		_valueList->weak=true; // MDH@11NOV2019: from now on a weak list i.e. elements are not added using assignValue but directly
	}
	Mlistelement* _valueListelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',owner_valueListelement); // both pointers NULL
	if(_valueListelement!=NULL){
		_value=(Mvalue*)CALLOC_1(sizeof(Mvalue),'X',owner_value); // MDH@07APR2020: should be 'X' not 'Y' // MDH@18MAY2020: immediately disown the Mvalue, because we return it 
		if(_value!=NULL){
			// shouldn't pose a problem now...
			_valueListelement->_value=_value;
			if(_valueList->_last!=NULL)_valueList->_last->_next=_valueListelement;else _valueList->_first=_valueListelement;
			_valueList->_last=_valueListelement;
			_valueList->numberOfElements++;
			// MDH@11NOV2019: by remembering the number of elements as index, removing intermediate elements will NOT prevent informing about what element was removed!!!
			_valueListelement->index=(++valueCount); // MDH@17JUN2020 replacing: _valueList->numberOfElements;
			if(descriptor!=NULL)
			if(amVerboseDebugging())
			output("Descriptor of value with id #%llu: '%s'.\n",_valueListelement->index,descriptor);
		}else // couldn't get a new value, so free the value list element immediately
			FREE_DISOWNED_1(_valueListelement,'l',owner);
	}
	if(NULL==_value)outputError("Failed to create value!"); // serious enough to report
	return _value;
}/* VALIDATED */
// MDH@01MAY2019: 'local' function for freeing a value

// MDH@28MAY2020: because we NEVER want anybody else then this module to call free_value, we made it static now
/**
 * @brief frees M value \p _value
 * @details only the garbage collector should call free_value() that's why it's declared static
 * @param _value
 */
static void free_value(Mvalue* _value/*,Mallocationowner owner*/){
	//////if(amVerbose()){output("Value of type '%s'",VALUETYPENAMES[_value->type]);outputValue(" to free: '",_value,"'.\n");}
	// I do not need to free the value itself, only the pointers inside it
	if(_value!=NULL){
		switch(_value->type){
			case VT_UNDEFINED:break;
			case VT_TOKEN:if(_value->value._token){FREE_TOKEN(_value->value._token,owner_value_data);_value->value._token=NULL;}break;
			case VT_INTEGER:if(_value->value._integer){FREE_INTEGER(_value->value._integer,owner_value_data);_value->value._integer=NULL;}break;
			case VT_BIGINTEGER:if(_value->value._biginteger){FREE_BIGINTEGER(_value->value._biginteger,owner_value_data);_value->value._biginteger=NULL;}break;
			case VT_DECIMAL:if(_value->value._decimal){FREE_DECIMAL(_value->value._decimal,owner_value_data);_value->value._decimal=NULL;}break;
			case VT_RATIONAL:if(_value->value._rational){FREE_RATIONAL(_value->value._rational,owner_value_data);_value->value._rational=NULL;}break;
			case VT_FLOAT:if(_value->value._float){FREE_FLOAT(_value->value._float,owner_value_data);_value->value._float=NULL;}break;
			case VT_TEXT:if(_value->value._text){FREE_TEXT(_value->value._text,owner_value_data);_value->value._text=NULL;}break;
			case VT_BYTES:if(_value->value._string){FREE_STRING(_value->value._string,owner_value_data);_value->value._string=NULL;}break; // MDH@13JUN2024
			case VT_ARRAY:if(_value->value._array){FREE_ARRAY(_value->value._array,owner_value_data);_value->value._array=NULL;}break;
			case VT_LIST:if(_value->value._list){FREE_LIST(_value->value._list,owner_value_data);_value->value._list=NULL;}break;
//			case VT_MATRIX:if(_value->value._matrix){FREE_MATRIX(_value->value._matrix,owner_value_data);_value->value._matrix=NULL;}break;
			case VT_MAP:if(_value->value._map){FREE_MAP(_value->value._map,owner_value_data);_value->value._map=NULL;}break;
			case VT_REFERENCE:if(_value->value._reference){FREE_REFERENCE(_value->value._reference,owner_value_data);_value->value._reference=NULL;}break; // MDH@04NOV2019: decrement the reference count to the variable
			case VT_FUNCTION:if(_value->value._function){FREE_FUNCTION(_value->value._function,owner_value_data);_value->value._function=NULL;}break;
			case VT_ENVIRONMENT:if(_value->value._environment){FREE_ENVIRONMENT(_value->value._environment,owner_value_data);_value->value._environment=NULL;}break;
			case VT_FILE:if(_value->value._file){FREE_FILE(_value->value._file,owner_value_data);_value->value._file=NULL;}break; // MDH@28SEP2020
			case VT_TIME:if(_value->value._time){FREE_TIME(_value->value._time,owner_value_data);_value->value._time=NULL;}break; // MDH@08DEC2020
			//case VT_USERFUNCTION:if(_value->value._userfunction)free_userfunction(_value->value._userfunction);break;
		}
		FREE_DISOWNED_1(_value,'X',owner_value);
		if(amVerboseDebugging())output("\tValue of type '%s' freed.\n",VALUETYPENAMES[_value->type]);
	}else
		outputBug("No value to free!");
}/* VALIDATED */

// can be asked to remove unused values
// TODO check whether it functions correctly (think so though)
/**
 * @brief returns the number of M values removed from the global M value list
 * @details any M value whose reference count is zero will be removed (and freed)
 * @param showInfo 
 * @return size_t 
 */
size_t getNumberOfRemovedValues(bool showInfo){Mallocationowner owner=getOwner(__LINE__);
	bool report=(showInfo||(M_MODULE_DEBUGGING&MM_VALUE));
	unsigned long long tofree=0,removed=0;
	if(_valueList!=NULL){
		if(report)
			output("Garbage collecting unused values.\n");
		Mallocationowner owner_valueListelement=Msubowner(owner_valueList,1);
		Mallocationowner owner_value=Msubowner(owner_valueList,2);
		Mlistelement* _valueListelement=_valueList->_first;
		if(report)
			output("Number of values to check: %llu.\n",_valueList->numberOfElements); // MDH@11NOV2019: no longer the actual number of elements to check
		// 1. free all M values in the list of which the reference count is zero nulling that value list element reference to the M value
		unsigned long long checked=0;
		while(_valueListelement!=NULL){
			checked++;
			if(_valueListelement->_value!=NULL){
				if(showInfo){
					output("Checking value #%llu with id %llu",checked,_valueListelement->index);
					/////outputValue(": '",_valueListelement->_value,"'");
					output(". ");
				}
				// if(showInfo)outputInfo("\tChecking the count!");
				if(_valueListelement->_value->count==0){ // unused
					if(showInfo)output("Freeing unused value #%llu of type '%s'.\n",checked,VALUETYPENAMES[_valueListelement->_value->type]);
					free_value(_valueListelement->_value);_valueListelement->_value=NULL; // essential to NULL so removing the value list elements below becomes possible
					// MDH@15MAR2023 moved below: tofree++;
				}else
				if(showInfo)outputInfo("Still in use!");
			}else
				output("%sNo value stored in value #%llu.\n",M_BUG_PREFIX,checked); // technically a bug not an error
			if(NULL==_valueListelement->_value)tofree++; // MDH@15MAR2023: every list element with NULL value is removable!!!
			_valueListelement=_valueListelement->_next;
		}
		if(report)
			output("Number of values checked: %llu.\nNumber of values to remove: %llu.\n",checked,tofree);
		// the list is now intact, are we going to correct the links??????

		// 2. if there are freed M values, i.e. 
		if(tofree>0){ // some values were freed
			Mlistelement* _firstValueListelement=NULL; // the first value list element to remain
			Mlistelement* _lastValueListelement=NULL; // the last value list element remaining
			Mlistelement* _nextValueListelement;
			_valueListelement=_valueList->_first;
			while(_valueListelement!=NULL){
				_nextValueListelement=_valueListelement->_next; // remember the next before freeing
				if(_valueListelement->_value){ // this one is too remain in the list
					if(NULL==_firstValueListelement)_firstValueListelement=_valueListelement;
					if(_lastValueListelement!=NULL)_lastValueListelement->_next=_valueListelement;
					_valueListelement->_next=NULL; // we can do this because we've already remembered the next one in the list at the start!!
					_lastValueListelement=_valueListelement; // remember the last element in the list (now pointing to nothing as the last element should!!!
				}else{ // this one is to be removed
					removed++;
					// let's be careful here!!!
					/* MDH@11NOV2019: let's decide NOT to decrement numberOfElements meaning that we NOW use numberOfElements to always have a unique index for every value ever added to it!!!
					if(_valueList->numberOfElements>0)_valueList->numberOfElements--;else output("BUG: Trying to free a value list element that is not counted!");  // one less element in the list!!!
					*/
					FREE_DISOWNED_1(_valueListelement,'l',owner_valueListelement);
					if(showInfo)if(removed%1000==0)output("M values removed so far: %llu.\n",removed);
				}
				// next to check!!!
				_valueListelement=_nextValueListelement;
			}
			if(report)
				output("Actual number of M values removed: %llu.\n",removed);
			// update the first and last in the list (could both be NULL!!!)
			_valueList->_first=_firstValueListelement;
			_valueList->_last=_lastValueListelement;
			// let's check how many we have left
			if(report){
				unsigned long long left=0;
				_valueListelement=_valueList->_first;
				while(_valueListelement){left++;_valueListelement=_valueListelement->_next;}
				long long unaccounted=checked;unaccounted-=(left+removed);
				output("Number of M values left: %llu (unaccounted: %lld).\n",left,unaccounted);
			}
		}
	}
	if(tofree>0){
		if(tofree>removed)
			output("%sFailed to free %llu unused value list elements.\n",M_WARNING_PREFIX,(tofree-removed));
		else 
		if(showInfo)
			outputInfo("All unused value list elements freed!");
	}
	return removed;
}/* VALIDATED */

/**
 * @brief returns the number of values in the global M value list
 * 
 * @return unsigned long long the number of M values in the global M value list
 */
unsigned long long getNumberOfValues(){
	return (_valueList!=NULL?_valueList->numberOfElements:0);
}/* VALIDATED */

/**
 * @brief decrements the reference count of M value \p _value
 * @details will fail if \p _value is NULL
 * @param _value 
 * @return true on success
 * @return false on failure
 */
bool decrementReferenceCount(Mvalue * const _value){Mallocationowner owner=getOwner(__LINE__);
	if(_value!=NULL){
		if(_value->count>0){(_value->count)--;return true;}
		Mstring* _valueText=owned_string(_getValueText(_value,false,false),owner);
		output("%sReference count of '%s' of type '%c' already zero.\n",M_BUG_PREFIX,string(_valueText),MUTABLEVALUETYPECHARS[_value->type]); // NOTE bugs should always be reported whether or not in amVerbose() mode or not!!!
		FREE_STRING(_valueText,owner);
	}else
	if(amVerbose())
		outputInfo("No value to decrement the reference count of.");
	return false;
}/* VALIDATED */
/**
 * @brief increments the reference count of M value \p _value
 * @details will fail if \p _value is NULL
 * @param _value 
 * @return true on success
 * @return false on failure
 */
bool incrementReferenceCount(Mvalue* _value){
	if(_value!=NULL){(_value->count)++;return true;}
	if(amVerbose())
		outputInfo("No value to increment the reference count of.");
	return false;
}/* VALIDATED */

// interface functions that use the above functions
// wrapping the different value type instances
// MDH@23MAY2020: why not return a global representing an undefined value (might be the first value in _valuelist)
/**
 * @brief returns the first M value on the global M value list which is considered the global undefined value
 * 
 * @return Mvalue* 
 */
Mvalue* _getUndefinedValue(){
	// MDH@15MAR2023: TODO should we actually disown that value, probably NOT
	//                TODO perhaps we change this function to create the _valueList if need be (as we now do in __value), so we can call _getUndefinedValue() instead?????
	return (_valueList!=NULL?DISOWNED(_valueList->_first->_value,owner_valueList):NULL);
	// MDH@23MAY2020 replacing: return (Mvalue*)CALLOC_1(sizeof(Mvalue),'U',-getOwner(__LINE__));
}/* VALIDATED */
/*
Mvalue* _getUserfunctionValue(Muserfunction* _userfunction,bool freeonfailure){
	if(!_userfunction)return NULL;
	Mvalue* _userfunctionValue=__value();
	if(_userfunctionValue){_userfunctionValue->type=VT_USERFUNCTION;_userfunctionValue->value._userfunction=_userfunction;}else if(freeonfailure)free_userfunction(_userfunction);
	return _userfunctionValue;
}// VALIDATED 
*/
// MDH@04NOV2019: no matter where the variable originates we can store it so it can be used elsewhere
#ifndef __PRODUCTION__
/**
 * @brief returns \p _reference owned by \p owner_reference
 * 
 * @param _reference 
 * @param owner_reference 
 * @return Mreference* \p _reference owned by \p owner_reference
 */
Mreference* owned_reference(Mreference* _reference,Mallocationowner owner_reference){return OWNED(_reference,owner_reference);}
/**
 * @brief returns \p _reference disowned by \p owner_reference
 * 
 * @param _reference 
 * @param owner_reference 
 * @return Mreference* \p _reference disowned by \p owner_reference
 */
Mreference* disowned_reference(Mreference* _reference,Mallocationowner owner_reference){return DISOWNED(_reference,owner_reference);}
#endif
void free_reference(Mreference* reference/*,Mallocationowner owner_reference*/){
	if(NULL==reference)return;
	if(reference->variable!=NULL){
		if(reference->variable->referencecount>0){
			reference->variable->referencecount--;
			reference->variable=NULL; // to be safe
		}else
			outputBug("Count of referenced variable already zero.");
	}
	FREE_1(reference,'Q'/*,owner_reference*/);
}
/**
 * @brief returns a M reference to \p _variable
 * 
 * @param _variable 
 * @return Mreference* a M reference to \p _variable
 */
Mreference* _getReference(Mvariable* _variable){Mallocationowner owner=getOwner(__LINE__);
	// MDH@11MAR2020: variable can now be NULL
	Mreference* _reference=CALLOC_1(sizeof(Mreference),'Q',owner);
	if(NULL==_reference)return NULL;
	_reference->variable=SUBOWNED(_variable,1); // TODO is this correct???? or should it be: owned_variable(_variable,subowner(owner,1));
	if(_variable!=NULL)_reference->referenceindex=(++_variable->referencecount); // MDH@11MAR2020: if variable is undefined, no reference count we can increment and assign
	return disowned_reference(_reference,owner);
}
/**
 * @brief returns a new M value wrapping M reference \p _reference
 * @details will 
 * @param _reference
 * @return Mvalue* a new M value wrapping M reference \p _reference
 */
Mvalue* _getValueOfReference(Mreference* _reference/*,Mallocationowner owner_reference*/){
	// MDH@19MAY2020: should we check whether _reference is ownable???????
	if(NULL==_reference){outputWarning("No reference to wrap.");return NULL;}
	Mvalue* _referenceValue=__value("reference");
	if(_referenceValue!=NULL){
		_referenceValue->type=VT_REFERENCE;
		_referenceValue->value._reference=(Misdisowned(_reference)?owned_reference(_reference,owner_value_data):_reference);
	}else
	if(Misdisowned(_reference))free_reference(_reference);
	return _referenceValue;
}
// MDH@26MAY2020: new contract, if NULL is returned _decimal is NOT bound and should be freed if desirable...
/**
 * @brief returns a new M value wrapping M decimal \p _decimal
 * @param _decimal
 * return Mvalue* a new M value wrapping M decimal \p _decimal
 */
Mvalue* _getValueOfDecimal(Mdecimal* _decimal/*,Mallocationowner owner_decimal*/){
	if(NULL==_decimal)return NULL;
	Mvalue* _decimalValue=__value("decimal");
	if(_decimalValue!=NULL){
		// outputDecimal("Wrapping decimal '",_decimal,"'.\n"); // DEBUG
		_decimalValue->type=VT_DECIMAL;
		_decimalValue->value._decimal=(Misdisowned(_decimal)?owned_decimal(_decimal,owner_value_data):_decimal);
	}else
	if(Misdisowned(_decimal))free_decimal(_decimal);
	return _decimalValue;
}/* VALIDATED */
/**
 * @brief returns a new M value wrapping M big integer \p _biginteger
 * @param _biginteger
 * @return a new M value wrapping M big integer \p _biginteger
 */
Mvalue* _getValueOfBiginteger(Mbiginteger* _biginteger/*,Mallocationowner owner_biginteger*/){
	if(NULL==_biginteger)return NULL;
	Mvalue* _bigintegerValue=__value("biginteger");
	if(_bigintegerValue){
		_bigintegerValue->type=VT_BIGINTEGER;
		_bigintegerValue->value._biginteger=(Misdisowned(_biginteger)?owned_biginteger(_biginteger,owner_value_data):_biginteger);
	}else
	if(Misdisowned(_biginteger))free_biginteger(_biginteger);
	return _bigintegerValue;
}/* VALIDATED */

/**
 * @brief returns a new M value wrapping long double \p ld
 * 
 * @param ld 
 * @return Mvalue* a new M value wrapping long double \p ld
 */
Mvalue* _getFloatValue(long double ld){
	if(amVerboseDebugging())output("Wrapping long double (float) '%.*Lf'.\n",DBL_DIG,ld);
	Mvalue* _floatValue=__value("long double");
	if(NULL==_floatValue)return NULL;
	_floatValue->type=VT_FLOAT;
	_floatValue->value._float=owned_float(_getFloat(ld),owner_value_data);
	return _floatValue;
}/* VALIDATED */
/**
 * @brief returns a new M value wrapping long long \p ll
 * 
 * @param ll 
 * @return Mvalue* a new M value wrapping long long \p ll
 */
Mvalue* _getIntegerValue(long long ll){Mallocationowner owner=getOwner(__LINE__);
	//////if(amVerboseDebugging())output("Wrapping integer '%lld'.\n",ll);
	Minteger* _integer=owned_integer(_getInteger(ll),owner);
	if(NULL==_integer)return NULL;
	Mvalue* _integerValue=__value("integer");
	if(NULL==_integerValue){free_integer(disowned_integer(_integer,owner));return NULL;}
	_integerValue->type=VT_INTEGER;
	_integerValue->value._integer=owned_integer(disowned_integer(_integer,owner),owner_value_data);
	return _integerValue;
}/* VALIDATED */

// MDH@25MAY2020: s is a constant character array that does not need change ownership (because it is supposed to be owned elsewhere or not owned)
/**
 * @brief returns a new M value wrapping the M text wrapping C string \p s
 * @param s
 * @result a new M value wrapping the M text wrapping C string \p s
 */
Mvalue* _getTextValue(char const * const s/*,bool freeonfailure*/){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==s)return NULL; // when no input, no go
	Mtext* _text=owned_text(_getText(s),owner);
	if(NULL==_text)return NULL;
	// output("Retrieving the text value of '%s' of length %zd.\n",s,strlen(s)); // DEBUG
	Mvalue* _textValue=__value("text"); // the result, when NULL check freeonfailure
	if(NULL==_textValue){free_text(disowned_text(_text,owner));return NULL;}
	_textValue->type=VT_TEXT;
	_textValue->value._text=owned_text(disowned_text(_text,owner),owner_value_data);
	return _textValue;
}/* VALIDATED */

/**
 * @brief returns an Mvalue* wrapping Mstring* \p str
 * @details typically used to store a (mutable) byte array read from a binary file (which may contain NUL characters so we need the length field to know how many bytes we have)
 * @param str 
 * @return Mvalue* an Mvalue* wrapping Mstring* \p str
 */
Mvalue* _getStringValue(Mstring const * const str){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==str)return NULL;
	bool disowned_string=Misdisowned(str); // determine if str is disowned, if it is we can free it when failing to wrap it!!
	Mvalue* _stringValue=__value("string");
	if(NULL==_stringValue){
		if(disowned_string)free_string(str);
		outputError("Failed to create a bytes value");
		return NULL;
	}
	_stringValue->type=VT_BYTES;
	_stringValue->value._string=(disowned_string?owned_string(str,owner_value_data):str);
	return _stringValue;
}

/**
 * @brief returns an M value wrapping the text in \p str
 * @details as opposed to _getTextValue() the text to wrap may contain NUL characters that are escaped
 * @param str the text to wrap
 * @return Mvalue* an M value wrapping the text in \p str
 */
Mvalue* _getStringTextValue(Mstring const * const str){Mallocationowner owner=getOwner(__LINE__);
	// if we bump into a NUL character before the end of str is reached, we should be escaping it to prevent from not getting the entire string
	char* s=string(str); // will also 'finish' str (i.e. write a NUL character where the length points to)
	if(NULL==s)return NULL;
	unsigned char* NULpos=(unsigned char*)strchr(str->_chars->chars,'\0');
	if(NULL==NULpos){outputBug("Missing NUL character!");return NULL;}
	// if there's a NUL character found, we need to do more!!!
	size_t l=str->length;
	if(l>=NULpos-str->_chars->chars)return _getTextValue(s);
	// we need to compose the result string that does not contain NUL characters anymore but can't contain single backslashes as well
	Mstring* _escapedText=owned_string(__string(),owner);
	if(NULL==_escapedText)return NULL;
	Mvalue* result=NULL;
	Mstring* p=_escapedText;
	char c;
	for(size_t i=0;i<l;i++){
		c=s[i];
		if(c=='\0'){p=string_append_char(p,'\\');c='0';}else
		if(c=='\\')p=string_append_char(p,'\\');
		p=string_append_char(p,c);
	}
	if(p!=NULL)result=_getTextValue(string(_escapedText));
	FREE_STRING(_escapedText,owner);
	return result;
}

/**
 * @brief returns a new M value wrapping a M text wrapping the C string of character \p c
 * 
 * @param _c 
 * @return Mvalue* a new M value wrapping a M text wrapping the C string of character \p c
 */
Mvalue* _getCharTextValue(char c,char quote){Mallocationowner owner=getOwner(__LINE__);
	Mtext* _text=owned_text(_getCharText(c,quote),owner);
	if(NULL==_text)return NULL;
	Mvalue* _textValue=__value("char");
	if(NULL==_textValue){free_text(disowned_text(_text,owner));return NULL;}
	_textValue->type=VT_TEXT;
	_textValue->value._text=owned_text(disowned_text(_text,owner),owner_value_data);
	return _textValue;
}/* VALIDATED */
// we can force all listelements to have the same type????
/**
 * @brief returns a new M value wrapping a new weak or strong M list with elements of type \p listValuetype 
 * 
 * @param listValuetype 
 * @param weak 
 * @param source 
 * @return Mvalue* a new M value wrapping a new weak or strong M list with elements of type \p listValuetype
 */
Mvalue* _getListValue(Mvaluetype listValuetype,bool weak,char const * const source){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=owned_list(__list(source!=NULL?source:"_getListValue"),owner);
	if(NULL==_list){
		// if(amVerboseDebugging())
			outputError("Failed to create a list.\n");
		return NULL;
	}
	Mvalue* _listValue=__value(weak?"weak list":"strong list");
	if(NULL==_listValue){
		FREE_LIST(_list,owner);
		return NULL;
	}
	_list->weak=weak;
	_list->valuetype=listValuetype; // register what type of elements this list should have	
	_listValue->type=VT_LIST;
	_listValue->value._list=owned_list(disowned_list(_list,owner),owner_value_data);
	return _listValue;
}/* VALIDATED */
/**
 * @brief returns an M list containing the indices of the elements of M list \p list
 * 
 * @param list 
 * @return Mlist* an M list containing the indices of the elements of M list \p list
 */
Mlist* _getListIndices(Mlist const * const list){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=owned_list(_getListOfType(VT_INTEGER),owner);
	if(NULL==_list)return NULL;
	Mlistelement* listelement=list->_first;
	while(listelement!=NULL){
		Mvalue* indexValue=_getIntegerValue(listelement->index);
		if(indexValue==NULL){
			outputError("Failed to create a list index value.");
			break;
		}
		if(appendedToList(_list,owner,indexValue,M_LL_INVALID)<=0){
			// NO need to free indexValue because it is a Value!!!
			outputError("Failed to append list element index");
			break;
		}
		listelement=listelement->_next;
	}
	return disowned_list(_list,owner);
}/* VALIDATED */

// MDH@19JUN2020: if something goes wrong reversing the list NULL is returned!!!
/**
 * @brief returns an M list containing the elements in M list \p list in reverse order
 * 
 * @param list 
 * @return Mlist* an M list containing the elements in M list \p list in reverse order
 */
Mlist* _getReversedList(Mlist const * const list){if(NULL==list)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=owned_list(_getListOfType(list->valuetype),owner); // make a list of the same type as the list argument
	if(NULL==_list){outputError("Failed to create a M list");return NULL;}
	Mlistelement* listelement=list->_first;
	while(listelement!=NULL){
		if(appendedToList(_list,owner,listelement->_value,0)<=0){FREE_LIST(_list,owner);return NULL;}
		listelement=listelement->_next;
	}
	return disowned_list(_list,owner);
}

// MDH@30MAR2020: the general idea of flattening a list is that all elements of the list are not lists anymore
//				let's assume that NULL means something went wrong, caller should take care of situation where value is NULL unless ?TODO? we allow putting a NULL value in the list
//				reversed tells _getFlattenedList to prepend instead of append
// MDH@06APR2020: passing in a flatten level that is decremented on each element and when it reaches 0 no flattening has to occur
/**
 * @brief returns a new M list with the successive non-list elements in the M list wrapped in \p value
 * 
 * @param value 
 * @param flattenLevel 
 * @param reversed whether or not to return the elements in reverse order
 * @return Mlist* a new M list with the successive non-list elements in the M list wrapped in \p value
 */
Mlist* _getFlattenedList(Mvalue const * const value,unsigned int flattenLevel,bool reversed){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=NULL;
	if(value!=NULL){
		if(amVerboseDebugging())
			outputValue("Flattening '",value,"'.\n");
		_list=owned_list(_getListOfType(VT_UNDEFINED),owner);
		if(_list!=NULL){
			bool success=true;
			if(value->type==VT_LIST){
				// we have to flatten all elements in the list obviously before we can actually add them to _list
				// if the list is empty we do NOT consider this an error, we simply do nothing
				Mlistelement* valueListelement=(value->value._list?value->value._list->_first:NULL);
				while(valueListelement!=NULL){
					// if there is a value associated _getFlattenedList should NOT return NULL (so NULL would represent an error)
					if(valueListelement->_value!=NULL){
						// MDH@31MAR2020: a little more effort to check if the value is a list in which case flattening is required, otherwise it is not
						// MDH@06APR2020: if flattenLevel>0 we need to add all list elements individually instead of all together
						if(flattenLevel>0&&valueListelement->_value->type==VT_LIST){
							Mlist* _subList=owned_list(_getFlattenedList(valueListelement->_value,(flattenLevel>0?flattenLevel-1:0),reversed),owner);
							if(_subList!=NULL){
								Mlistelement* valueSubListelement=_subList->_first;
								while(valueSubListelement!=NULL){
									// if supposed to return the prepend instead of append pass in 0 instead of M_LL_INVALID
									if(valueSubListelement->_value!=NULL)if(appendedToList(_list,owner,valueSubListelement->_value,(reversed?0:M_LL_INVALID))<=0){success=false;break;}
									valueSubListelement=valueSubListelement->_next;
								}
								// ALWAYS free _sublist
								FREE_LIST(_subList,owner);
							}else
								success=false;
						}else
						if(appendedToList(_list,owner,valueListelement->_value,(reversed?0:M_LL_INVALID))<=0)success=false;
						if(!success)break; // do NOT continue with appending when failing to do so
					}
					valueListelement=valueListelement->_next;
				}
			}else // a single element to add to the list
			if(appendedToList(_list,owner,value,M_LL_INVALID)<=0)success=false;
			if(!success){FREE_LIST(_list,owner);_list=NULL;} // on failure release the list
		}	
	}
	if(amVerboseDebugging()){
		if(_list!=NULL){if(flattenLevel>0)outputList("Flattened to '",_list,"'.\n");else outputList("Converted to '",_list,"'.\n");}
	}
	return disowned_list(_list,owner);
}
/*
Mmatrix* disowned_matrix(Mmatrix* _matrix,Mallocationowner owner_matrix){
	if(NULL==_matrix)return NULL;
	if(_matrix->_array!=NULL)disowned_array(_matrix->_array,owner_matrix);
	return DISOWNED(_matrix,owner_matrix);
}
Mmatrix* owned_matrix(Mmatrix* _matrix,Mallocationowner owner_matrix){
	if(NULL==_matrix)return NULL;
	if(_matrix->_array!=NULL)owned_array(_matrix->_array,Msubowner(owner_matrix,1));
	return OWNED(_matrix,owner_matrix);
}

Mmatrix* __matrix(long long numberOfRows,long long numberOfColumns){Mallocationowner owner=getOwner(__LINE__);
	if(numberOfRows<=0||numberOfColumns<=0)return NULL;
	Mmatrix* _matrix=(Mmatrix*)CALLOC_1(sizeof(Mmatrix),'M',owner);
	if(NULL==_matrix)return NULL;
	Marray* _array=_getArray("matrix",numberOfRows*numberOfColumns,NULL);
	if(NULL==_array){free_matrix(_matrix,owner);return NULL;}
	_matrix->_array=owned_array(_array,Msubowner(owner,1));
	_matrix->numberOfRows=numberOfRows;
	_matrix->numberOfRows=numberOfColumns;
	return disowned_matrix(_matrix,owner);
}
Mmatrix* _getMatrix(long long numberOfRows,long long numberOfColumns,Mvalue* defaultElementValue){Mallocationowner owner=getOwner(__LINE__);
	Mmatrix* _matrix=owned_matrix(__matrix(numberOfRows,numberOfColumns),owner);
	if(NULL==_matrix)return NULL;
	size_t left=_matrix->numberOfRows*_matrix->numberOfColumns;
	while(left-->0)
		assignValue(&_matrix->_array->values[left],defaultElementValue);
	return disowned_matrix(_matrix,owner);
}

void free_matrix(Mmatrix* _matrix,Mallocationowner owner_matrix){
	if(_matrix!=NULL){
		if(_matrix->_array!=NULL)FREE_ARRAY(_matrix->_array,Msubowner(owner_matrix,1));
		FREE_1(_matrix,'M');
	}
}
*/

/**
 * @brief return the first scalar in M value \p value
 * 
 * @param value 
 * @return Mvalue* the first scalar in M value \p value
 */
Mvalue* getFirstScalarValue(Mvalue* value){
	if(value!=NULL){
		if(value->type==VT_MAP)return NULL; // can't go into a map
		if(value->type==VT_LIST){
			Mlistelement* listelement=(value->value._list!=NULL?value->value._list->_first:NULL);
			while(listelement!=NULL){
				Mvalue* firstScalarValue=getFirstScalarValue(listelement->_value);
				if(firstScalarValue!=NULL)return firstScalarValue; // if this element is a scalar (i.e. defined and not a map or a list)
				listelement=listelement->_next;
			}
			return NULL;
		}
		// MDH@15MAR2023: an array is also a composite non-scalar structure
		if(value->type==VT_ARRAY){
			Mvalue** values=(value->value._array!=NULL?value->value._array->values:NULL);
			if(values!=NULL){
				unsigned long long l=0;
				while(l<value->value._array->numberOfElements){
					Mvalue* firstScalarValue=getFirstScalarValue(value->value._array->values[l]);
					if(firstScalarValue!=NULL)return firstScalarValue;
					l++;
				}
			}
			return NULL;
		}
	}
	return value;
}

/**
 * @brief returns the key at index \p mapIndex of \p map
 * 
 * @param map 
 * @param mapIndex 
 * @return char* the key at index \p mapIndex of \p map
 */
char* getMapKey(Mmap const * const map,size_t mapIndex){ // MDH@30JUL2024
	if(map!=NULL&&mapIndex>0&&mapIndex<=map->numberOfElements){
		Mmapelement* mapelement=map->_first;
		while(mapelement!=NULL){
			if(--mapIndex==0){
				if(mapelement->_variable!=NULL&&mapelement->_variable->_name!=NULL)
					return mapelement->_variable->_name->chars;
				break;
			}
			mapelement=mapelement->_next;
		}
	}
	return NULL;
}

/**
 * @brief returns a new M list with the attributes of M map \p map
 * 
 * @param map 
 * @return Mlist* a new M list with the attributes of M map \p map
 */
Mlist* _getMapAttributes(Mmap const * const map){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _list=owned_list(_getListOfType(VT_TEXT),owner);
	if(NULL==_list)return NULL;
	Mmapelement* mapelement=map->_first;
	while(mapelement!=NULL){
		// I guess we'll have to duplicate the attribute name because it will be wrapped inside a Value
		// this is a bit of an issue because typically text should be enquoted
		Mstring* _attributeName=owned_string(_getString("'"),owner);
		if(NULL==_attributeName){outputError("Failed to duplicate a map attribute name");break;}
		string_append(_attributeName,mapelement->_variable->_name->chars); // MDH@17APR2020: char* _name replaced by Mchars* _name // append the attribute name
		Mvalue* attributeValue=_getTextValue(string(_attributeName));
		FREE_STRING(_attributeName,owner);
		if(NULL==attributeValue){outputError("Failed to store a map attribute name");break;}
		if(appendedToList(_list,owner,attributeValue,M_LL_INVALID)<=0){
			// NO need to free indexValue because it is a Value!!!
			outputError("Failed to append map attribute name");break;
		}
		mapelement=mapelement->_next;
	}
	return disowned_list(_list,owner);
}

// MDH@23MAY2020: although a value is (weakly but permanently) stored in _valuelist we can set its owner to the function that created it
/**
 * @brief returns a new M value wrapping a new M map with value elements of type \p mapValuetype
 * 
 * @param mapValuetype 
 * @param weak 
 * @param source 
 * @return Mvalue* a new M value wrapping a new M map with value elements of type \p mapValuetype
 */
Mvalue* _getMapValue(Mvaluetype mapValuetype,bool weak,char const * const source){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
	if(NULL==_map)return NULL;
	Mvalue* _mapValue=__value(weak?"weak map":"strong map");
	if(_mapValue!=NULL){
		_map->weak=weak;
		_map->valuetype=mapValuetype;
		_mapValue->type=VT_MAP;
		_mapValue->value._map=owned_map(disowned_map(_map,owner),owner_value_data);
	}else
		FREE_MAP(_map,owner);
	return _mapValue;
}/* VALIDATED */

// some other wrappers
// if something is wrapped in a value it should get the ownership of the value
// the question now is what owner_integer actually means
// the point is that we cannot free an integer or obtain ownership if owner_integer is not correct
// however, it should be possibly to bind _integer to the value that is created in which case we should be able to obtain ownership
// meaning that whatever we receive should be a disowned integer unless we do a super own
/**
 * @brief returns a new M value wrapping M integer \p _integer
 * @details will free \p integer if not wrapped if \p integer is currently disowned
 * @param _integer
 * @result a new M value wrapping M integer \p _integer
 */
Mvalue* _getValueOfInteger(Minteger* _integer/*,Mallocationowner owner_integer*/){
	if(NULL==_integer)return NULL;
	bool disowned_integer=Misdisowned(_integer);
	Mvalue* _value=__value("integer"); // make the value create have the same owner as the integer
	if(_value!=NULL)
	{_value->value._integer=(disowned_integer?owned_integer(_integer,owner_value_data):_integer);_value->type=VT_INTEGER;}
	else
	if(disowned_integer)
		free_integer(_integer);
	return _value;
}/* VALIDATED */
/**
 * @brief returns a new M value wrapping M float \p _float
 * @details will free \p _float if not wrapped if \p _float is currently disowned
 * @param _float
 * @result a new M value wrapping M float \p _float
 */
Mvalue* _getValueOfFloat(Mfloat* _float/*,Mallocationowner owner_float*/){
	if(NULL==_float)return NULL;
	bool disowned_float=Misdisowned(_float);
	Mvalue* _value=__value("float");
	if(_value!=NULL)
	{_value->value._float=(disowned_float?owned_float(_float,owner_value_data):_float);_value->type=VT_FLOAT;}
	else 
	if(disowned_float)
		free_float(_float);
	return _value;
}/* VALIDATED */
/**
 * @brief returns a new M value wrapping M map \p _map
 * @details will free \p _map if not wrapped if \p _map is currently disowned
 * @param _map
 * @result a new M value wrapping M map \p _map
 */
Mvalue* _getValueOfMap(Mmap* _map/*,Mallocationowner owner_map*/){
	if(NULL==_map)return NULL;
	bool disowned_map=Misdisowned(_map);
	Mvalue* _value=__value("map");
	if(_value!=NULL)
	{_value->value._map=(disowned_map?owned_map(_map,owner_value_data):_map);_value->type=VT_MAP;}
	else 
	if(disowned_map)free_map(_map);
	return _value;
}/* VALIDATED */
/**
 * @brief returns a new M value wrapping M token \p _token
 * @details will free \p _token if not wrapped if \p _token is currently disowned
 * @param _token
 * @result a new M value wrapping M token \p _token
 */
Mvalue* _getValueOfToken(Mtoken* _token/*,Mallocationowner owner_token*/){
	if(NULL==_token)return NULL;
	bool disowned_token=Misdisowned(_token);
	Mvalue* _value=__value("token");
	if(_value!=NULL)
	{_value->value._token=(disowned_token?owned_token(_token,owner_value_data):_token);_value->type=VT_TOKEN;}
	else 
	if(disowned_token)free_token(_token);
	return _value;
}/* VALIDATED */
// MDH@07DEC2020: why wasn't this defined before??????
Mvalue* _getValueOfText(Mtext* _text){
	if(NULL==_text)return NULL;
	bool disowned_text=Misdisowned(_text); // if _text is currently disowned we're going to free it if we fail to wrap it in a value
	Mvalue* _value=__value("text");
	if(_value!=NULL)
	{_value->value._text=(disowned_text?owned_text(_text,owner_value_data):_text);_value->type=VT_TEXT;}
	else
	if(disowned_text)free_text(_text);
	return _value;
}/* VALIDATED */

/* TODO move elsewhere
Mvalue* _getTokenValue(Mtoken* _token,bool freeonfailure){
	if(!_token)return NULL;
	Mvalue* _tokenValue=__value();
	if(_tokenValue){_tokenValue->type=VT_TOKEN;_tokenValue->value._token=_token;}else if(freeonfailure)FREE_TOKEN(_token);
	return _tokenValue;
}*/

// MDH@09JUN2020: interface between _getVariable (now requiring a Mchars name) and all that still use a char* thing
/**
 * @brief returns a new M variable with name \p name and value type \p valuetype
 * 
 * @param name 
 * @param valuetype 
 * @param unlockCode sets the immutable flag of the new M variable returned
 * @param owner_variable 
 * @return Mvariable* a new M variable with name \p name and value type \p valuetype
 */
Mvariable* _getVariableWithName(char const * const name,Mvaluetype valuetype,long long unlockCode,Mallocationowner owner_variable){
	// MDH@21OCT2020: A HA we should allow the name of a variable to be empty (as in maps)
	if(NULL==name/*||strlen(name)==0*/)return NULL;//Mallocationowner owner=getOwner(__LINE__);
	Mchars* name_chars=_getChars(name);
	if(NULL==name_chars)return NULL;
	Mvariable* _variable=owned_variable(_getVariable(name_chars,valuetype,unlockCode),owner_variable);
	if(NULL==_variable)free_chars(name_chars,1,strlen(name)+1,'\''); // MDH@15MAR2023: TODO we should change free_chars, to NOT need any arguments since we can determine these from the contained C string pointer!
	return _variable;
}
// helper function to create a parameter map with a single value
// the following is a nuisance
/**
 * @brief returns a new M map containing a property with name \p name and value \p _floatValue
 * 
 * @param name 
 * @param _floatValue 
 * @return Mmap* a new M map containing a property with name \p name and value \p _floatValue
 */
Mmap* _getFloatMap(char* name,Mvalue* _floatValue){Mallocationowner owner=getOwner(__LINE__);
	if(name!=NULL&&_floatValue!=NULL){
		Mvariable* _realVariable=_getVariableWithName(name,VT_FLOAT,true,Msubowner(owner,2)); // MDH@11JUN2020: _getChars(name) returns a disowned pointer that we need in _getVariable()
		if(_realVariable!=NULL){
			Mmapelement* _mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			if(_mapelement!=NULL){
				Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					assignValue(&_realVariable->_value,_floatValue); //////////////incrementReferenceCount(_realValue); // now bound to the real variable!!!// ESSENTIAL to prevent loosing _zeroIntegerValue!!!
					_mapelement->_variable=_realVariable;
					_map->numberOfElements=1;
					_map->_first=_mapelement;
					_map->_last=_mapelement;
					if(amVerboseDebugging())
						outputInfo("Returning the single float map!");
					return disowned_map(_map,owner);
				}
				outputError("Failed to create the float variable map");
				FREE_MAPELEMENT(_mapelement,false,owner);
			}else
				outputError("Failed to create the float variable map element");
			FREE_VARIABLE(_realVariable,false,owner);
		}else
			outputError("Failed to create the float variable name");
	}
	return NULL;
}/* VALIDATED */
/**
 * @brief returns a new M map containing a property with name \p name with a value of type undefined (i.e. without a value)
 * 
 * @param name 
 * @return Mmap* a new M map containing a property with name \p name with a value of type undefined (i.e. without a value)
 */
Mmap* _getMap(char* name){if(NULL==name)return NULL;Mallocationowner owner=getOwner(__LINE__);
	Mvariable* _variable=_getVariableWithName(name,VT_UNDEFINED,true,Msubowner(owner,2));
	if(_variable!=NULL){
		Mmapelement* _mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
		if(_mapelement!=NULL){
			Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
			if(_map!=NULL){
				_map->numberOfElements=1;
				_mapelement->_variable=_variable; // TODO we can do this better given the new disowned_map usage
				_map->_first=_mapelement;
				_map->_last=_mapelement;
				return disowned_map(_map,owner);
			}
			FREE_MAPELEMENT(_mapelement,false,owner); // MDH@11NOV2019: no need to call free_mapelement() 
		}
		FREE_VARIABLE(_variable,false,owner);
	}else
		output("%sFailed to create map '%s'.\n",M_ERROR_PREFIX,name);
	return NULL;
}/* VALIDATED */
/**
 * @brief returns a new M map copy of M map \p map
 * 
 * @param map 
 * @return Mmap* a new M map copy of M map \p map
 */
Mmap* _getMapCopy(Mmap const * const map){Mallocationowner owner=getOwner(__LINE__); // creates a 'deep' copy
	if(map!=NULL){
		Mmap* _map=owned_map(_getMapOfType(map->valuetype),owner);
		if(_map!=NULL){
			Mvariable *mapelementVariable,*_mapelementVariable=NULL;
			Mmapelement *mapelement=map->_first,*_mapelement=NULL;
			while(mapelement!=NULL){
				mapelementVariable=mapelement->_variable;
				if(mapelementVariable!=NULL){
					_mapelement=CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1)); // new map element to hold a copy
					if(_mapelement!=NULL){
						// create a variable with the same name and value as the variable in mapelement
						// MDH@12MAR2020 OOPS: why would we make the copy ALWAYS immutable: replacing true by mapelementVariable->immutable
						_mapelement->_variable=_getVariableWithName(mapelementVariable->_name->chars,mapelementVariable->valuetype,mapelementVariable->unlockCode/*true*/,Msubowner(owner,2));
						if(_mapelement->_variable!=NULL){
							assignValue(&_mapelement->_variable->_value,mapelementVariable->_value); // 'copy' the value over
							if(_map->_last!=NULL)_map->_last->_next=_mapelement; // make the current last point to the new last
							_map->_last=_mapelement; // replace current last by the new last
							if(NULL==_map->_first)_map->_first=_map->_last; // initialize first if necessary
							_map->numberOfElements++; // count one more
						}else
							output("%sFailed to copy the map attribute name '%s'.\n",M_ERROR_PREFIX,mapelementVariable->_name->chars);
					}else
						output("%sFailed to copy map attribute '%s'.\n",M_ERROR_PREFIX,mapelementVariable->_name->chars);
				}
				mapelement=mapelement->_next;
			}
			return disowned_map(_map,owner);
		}else
			outputError("Failed to create a map");
	}
	return NULL;
}
/**
 * @brief returns a new M list copy of M list \p list
 * 
 * @param list 
 * @return Mlist* a new M list copy of M list \p list
 */
Mlist* _getListCopy(Mlist const * const list){Mallocationowner owner=getOwner(__LINE__); // creates a 'deep' copy
	if(list!=NULL){
		Mlist* _list=owned_list(_getListOfType(list->valuetype),owner);
		if(_list!=NULL){
			Mlistelement *listelement=list->_first,*_listelement=NULL;
			while(listelement!=NULL){
				_listelement=CALLOC_1(sizeof(Mlistelement),'l',Msubowner(owner,1));
				if(_listelement!=NULL){
					// 'copy' the value over, here we have the same problem as with copying any other value: if the value to copy is composite (a list or a map) we should copy by value i.e. point to a new map or list and not to the original
					// technically we could let assignValue() take care of that 
					assignValue(&_listelement->_value,listelement->_value); // no need to 'copy' the value itself, we only need to make another (strong) reference to that value
					_listelement->index=listelement->index;
					if(_list->_last!=NULL)_list->_last->_next=_listelement;
					_list->_last=_listelement;
					if(NULL==_list->_first)_list->_first=_list->_last;
					_list->numberOfElements++;
				}else 
					outputError("Failed to copy a list element");
				listelement=listelement->_next;
			}
			return disowned_list(_list,owner);
		}else
			outputError("Failed to create a list");
	}
	return NULL;
}

// MDH@24MAY2020: more convenient to be able to make any map with a certain number of arguments with names and types
/**
 * @brief returns a one argument map with name \p name and attribute value type \p valuetype
 * 
 * @param name 
 * @param valuetype 
 * @return Mmap* a one argument map with name \p name and attribute value type \p valuetype
 */
static Mmap* _getOneArgumentMap(char* name,Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
	// NOTE wait with filling the single integer value map until we have all the ingredients
	if(name!=NULL&&strlen(name)>0){
		Mvariable* _variable=_getVariableWithName(name,valuetype,true,Msubowner(owner,2));
		if(_variable!=NULL){
			Mmapelement* _mapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			if(_mapelement!=NULL){
				Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					_mapelement->_variable=_variable;
					_map->numberOfElements=1;
					_map->_first=_mapelement;
					if(amVerboseDebugging())outputInfo("Returning the single integer map!");
					return disowned_map(_map,owner);
				}
				outputError("Failed to create the one variable map");
				FREE_MAPELEMENT(_mapelement,false,owner);
			}else
				outputError("Failed to create the one variable map element");
			FREE_VARIABLE(_variable,false,owner);
		}else
			outputError("Failed to create the variable of the one variable map");
	}
	return NULL;
}
/**
 * @brief returns a new M map of type VT_INTEGER initialized to contain an attribute with name \p name and value \p _integerValue
 * 
 * @param name 
 * @param _integerValue 
 * @return Mmap* a new M map of type VT_INTEGER initialized to contain an attribute with name \p name and value \p _integerValue
 */
Mmap* _getIntegerMap(char* name,Mvalue* _integerValue){Mallocationowner owner=getOwner(__LINE__);
	// NOTE wait with filling the single integer value map until we have all the ingredients
	if(NULL==name||strlen(name)==0)return NULL;
	Mmap* _integerMap=owned_map(_getOneArgumentMap(name,VT_INTEGER),owner);
	if(_integerMap!=NULL&&_integerValue!=NULL){
		assignValue(&_integerMap->_first->_variable->_value,_integerValue);
		return disowned_map(_integerMap,owner);
	}
	if(_integerMap!=NULL)FREE_MAP(_integerMap,owner);
	return NULL;
}/* VALIDATED */
/**
 * @brief returns a new M map of type VT_LIST initialized with an attribute with name \p name and value \p _listValue
 * 
 * @param name 
 * @param _listValue 
 * @return Mmap* a new M map of type VT_LIST initialized with an attribute with name \p name and value \p _listValue
 */
Mmap* _getListMap(char* name,Mvalue* _listValue){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==name||strlen(name)==0)return NULL;
	if(NULL==_listValue){
		// if(amVerboseDebugging())
			output("Unable to create list map: no list value to put in list.\n");
		return NULL;
	}
	Mmap* _listMap=owned_map(_getOneArgumentMap(name,VT_LIST),owner);
	if(NULL==_listMap){
		// if(amVerboseDebugging())
		   output("%sFailed to create a one argument list map.\n",M_ERROR_PREFIX);
		return NULL;
	}
	assignValue(&_listMap->_first->_variable->_value,_listValue);
	return disowned_map(_listMap,owner);
}/* VALIDATED */
/**
 * @brief returns a new map of type VT_MAP initialized with an attribute with name \p name and value \p _mapValue
 * 
 * @param name 
 * @param _mapValue 
 * @return Mmap* a new map of type VT_MAP initialized with an attribute with name \p name and value \p _mapValue
 */
Mmap* _getMapMap(char* name,Mvalue* _mapValue){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==name||strlen(name)==0)return NULL;
	if(NULL==_mapValue){
		// if(amVerboseDebugging())
			output("Unable to create map map: no map value to put in map.\n");
		return NULL;
	}
	Mmap* _mapMap=owned_map(_getOneArgumentMap(name,VT_MAP),owner);
	if(NULL==_mapMap){
		// if(amVerboseDebugging())
		   output("%sFailed to create a one argument map map.\n",M_ERROR_PREFIX);
		return NULL;
	}
	assignValue(&_mapMap->_first->_variable->_value,_mapValue);
	return disowned_map(_mapMap,owner);
}/* VALIDATED */

/**
 * @brief returns a two argument M map with attributes with name \p name1 and \p name2 with value of value types \p valuetype1 and \p valuetype2 respectively
 * 
 * @param name1 
 * @param name2 
 * @param valuetype1 
 * @param valuetype2 
 * @return Mmap* a two argument M map with attributes with name \p name1 and \p name2 with value of value types \p valuetype1 and \p valuetype2 respectively
 */
static Mmap* _getTwoArgumentMap(char* name1,char* name2,Mvaluetype valuetype1,Mvaluetype valuetype2){Mallocationowner owner=getOwner(__LINE__);
	if(name1!=NULL&&name2!=NULL){
		if(strlen(name1)&&strlen(name2)&&strcmp(name1,name2)){
			Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			if(_mapelement1!=NULL&&_mapelement2!=NULL){
				Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					_mapelement1->_variable=_getVariableWithName(name1,valuetype1,true,Msubowner(owner,2));
					_mapelement2->_variable=_getVariableWithName(name2,valuetype2,true,Msubowner(owner,2));
					if(_mapelement1->_variable!=NULL&&_mapelement2->_variable!=NULL){
						_map->_first=_mapelement1;
						_mapelement1->_next=_mapelement2;
						_map->_last=_mapelement2;
						_map->numberOfElements=2;
						return disowned_map(_map,owner);
					}
					outputError("Failed to create both map element variables");
					FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
				}else
					outputError("Failed to create a map");
			}else
				outputError("Failed to create both integer map elements");
			// either map element might have been created and we need to release them
			FREE_MAPELEMENT(_mapelement1,false,owner);
			FREE_MAPELEMENT(_mapelement2,false,owner);
		}else
			output("%sTwo argument map element names '%s' and '%s' undefined or the same.\n",M_ERROR_PREFIX,name1,name2);
	}else
		outputError("Not both two map element names defined");
	return NULL;	
}
/**
 * @brief returns a two argument M map containing an integer and a boolean attribute with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing an integer and a boolean attribute with name \p name1 and \p name2 respectively
 */
Mmap* _getIntegerBooleanMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_INTEGER,VT_INTEGER);}/* VALIDATED */
/**
 * @brief returns a two argument M map containing two string value attributes with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing two string value attributes with name \p name1 and \p name2 respectively
 */
Mmap* _getStringStringMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_TEXT,VT_TEXT);}/* VALIDATED */
/**
 * @brief returns a two argument M map containing two float value attributes with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing two float value attributes with name \p name1 and \p name2 respectively
 */
Mmap* _getFloatFloatMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_FLOAT,VT_FLOAT);}/* VALIDATED */
/**
 * @brief returns a two argument M map containing a map and a token attribute with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing a map and a token attribute with name \p name1 and \p name2 respectively
 */
Mmap* _getMapTokenMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_MAP,VT_TOKEN);}/* VALIDATED */
/**
 * @brief returns a two argument M map containing two token value attributes with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing two token attribute with name \p name1 and \p name2 respectively

 */
Mmap* _getTokenTokenMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
/**
 * @brief returns a two argument M map containing a list and a function attribute with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing a list and a function attribute with name \p name1 and \p name2 respectively
 */
Mmap* _getListFunctionMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_LIST,VT_FUNCTION);}/* VALIDATED */
// MDH@03NOV2020: allowing to specify the sort method to use (now 't' for timsort, 'm' for mergesort and 'q' (default) for quicksort)
/**
 * @brief returns a two argument M map containing a list and a text attribute with name \p name1 and \p name2 respectively
 * 
 * @param name1 
 * @param name2 
 * @return Mmap* a two argument M map containing a list and a text attribute with name \p name1 and \p name2 respectively
 */
Mmap* _getListTextMap(char* name1,char* name2){return _getTwoArgumentMap(name1,name2,VT_LIST,VT_TEXT);}/* VALIDATED */

/**
 * @brief returns a new M map containing three attributes with names \p name1 , \p name2 and \p name3 with values of type \p valuetype1 , \p valuetype2 and \p valuetype3 respectively
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param valuetype1 
 * @param valuetype2 
 * @param valuetype3 
 * @return Mmap* a new M map containing three attributes with names \p name1 , \p name2 and \p name3 with values of type \p valuetype1 , \p valuetype2 and \p valuetype3 respectively
 */
Mmap* _getThreeArgumentMap(char const * const name1,char const * const name2,char const * const name3,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3){Mallocationowner owner=getOwner(__LINE__);
	if(name1!=NULL&&name2!=NULL&&name3!=NULL){
		if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name2,name3)){
			Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			if(_mapelement1!=NULL&&_mapelement2!=NULL&&_mapelement3!=NULL){
				Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					_mapelement1->_variable=_getVariableWithName(name1,valuetype1,true,Msubowner(owner,2));
					_mapelement2->_variable=_getVariableWithName(name2,valuetype2,true,Msubowner(owner,2));
					_mapelement3->_variable=_getVariableWithName(name3,valuetype3,true,Msubowner(owner,2));
					if(_mapelement1->_variable!=NULL&&_mapelement2->_variable!=NULL&&_mapelement3->_variable!=NULL){
						_map->_first=_mapelement1;
						_mapelement1->_next=_mapelement2;
						_mapelement2->_next=_mapelement3;
						_map->_last=_mapelement3;
						_map->_last->_next=NULL; // MDH@04AUG2023: why would we need this???????
						_map->numberOfElements=3;
						// output("Returning disowned map of '%s', '%s' and '%s'.\n",name1,name2,name3); // DEBUG
						return disowned_map(_map,owner);
					}
					FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
				}
			}
			// either map element might have been created and we need to release them
			FREE_MAPELEMENT(_mapelement1,false,owner);
			FREE_MAPELEMENT(_mapelement2,false,owner);
			FREE_MAPELEMENT(_mapelement3,false,owner);
		}
	}
	return NULL;
}
/**
 * @brief returns a new three attributes M map with values of type string, map and token respectively
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with values of type string, map and token respectively
 */
Mmap* _getStringMapTokenMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_TEXT,VT_MAP,VT_TOKEN);}/* VALIDATED */
//Mmap* _getValueTokenTokenMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_UNDEFINED,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
/**
 * @brief returns a new three attributes M map with integer values
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with integer values
 */
Mmap* _getThreeIntegerMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_INTEGER,VT_INTEGER,VT_INTEGER);}/* VALIDATED */
/**
 * @brief returns a new three attributes M map with values of type list, value and integer respectively
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with values of type list, value and integer respectively
 */
Mmap* _getListValueIntegerMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_LIST,VT_UNDEFINED,VT_INTEGER);}/* VALIDATED */
/**
 * @brief returns a new three attributes M map with undefined values
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with undefined values
 */
Mmap* _getValueValueValueMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_UNDEFINED,VT_UNDEFINED,VT_UNDEFINED);}/* VALIDATED */
/**
 * @brief returns a new three attributes M map with values of type map, map and list respectively
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with values of type map, map and list respectively
 */
Mmap* _getMapMapListMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_MAP,VT_MAP,VT_LIST);}/* VALIDATED */
/**
 * @brief returns a new three attributes M map with values of type list, function and value respectively
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @return Mmap* a new three attributes M map with values of type list, function and value respectively
 */
Mmap* _getListFunctionValueMap(char* name1,char* name2,char* name3){return _getThreeArgumentMap(name1,name2,name3,VT_LIST,VT_FUNCTION,VT_UNDEFINED);}/* VALIDATED */

/**
 * @brief returns a new four attributes M map with values of the given value types
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param name4
 * @param valuetype1
 * @param valuetype2
 * @param valuetype3
 * @param valuetype4
 * @return Mmap* a new four attributes M map with values of the given value types
 */
Mmap* _getFourArgumentMap(char* name1,char* name2,char* name3,char *name4,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3,Mvaluetype valuetype4){Mallocationowner owner=getOwner(__LINE__);
	if(name1!=NULL&&name2!=NULL&&name3!=NULL&&name4!=NULL){
		if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
			strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name3,name4)){
			Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			Mmapelement* _mapelement4=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',Msubowner(owner,1));
			if(_mapelement1!=NULL&&_mapelement2!=NULL&&_mapelement3!=NULL&&_mapelement4!=NULL){
				Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					_mapelement1->_variable=_getVariableWithName(name1,valuetype1,true,Msubowner(owner,2));
					_mapelement2->_variable=_getVariableWithName(name2,valuetype2,true,Msubowner(owner,2));
					_mapelement3->_variable=_getVariableWithName(name3,valuetype3,true,Msubowner(owner,2));
					_mapelement4->_variable=_getVariableWithName(name4,valuetype4,true,Msubowner(owner,2));
					if(_mapelement1->_variable!=NULL&&_mapelement2->_variable!=NULL&&_mapelement3->_variable!=NULL&&_mapelement4->_variable!=NULL){
						_map->_first=_mapelement1;
						_mapelement1->_next=_mapelement2;
						_mapelement2->_next=_mapelement3;
						_mapelement3->_next=_mapelement4;
						_map->_last=_mapelement4;
						_map->numberOfElements=4;
						return disowned_map(_map,owner);
					}
					FREE_MAP(_map,owner); // failed to create the two map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
				}
			}
			// either map element might have been created and we need to release them
			FREE_MAPELEMENT(_mapelement1,false,owner);
			FREE_MAPELEMENT(_mapelement2,false,owner);
			FREE_MAPELEMENT(_mapelement3,false,owner);
			FREE_MAPELEMENT(_mapelement4,false,owner);
		}
	}
	return NULL;
}
/**
 * @brief returns a new four attribute M map with given names containing four token values
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param name4 
 * @return Mmap* a new four attribute M map with given names containing four token values
 */
Mmap* _getTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4){return _getFourArgumentMap(name1,name2,name3,name4,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
/**
 * @brief returns a new four attribute M map with given names containing one ordinary and three token values
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param name4 
 * @return Mmap* a new four attribute M map with given names containing one ordinary and three token values
 */
Mmap* _getValueTokenTokenTokenMap(char* name1,char* name2,char* name3,char* name4){return _getFourArgumentMap(name1,name2,name3,name4,VT_UNDEFINED,VT_TOKEN,VT_TOKEN,VT_TOKEN);}/* VALIDATED */

/**
 * @brief returns a new five attribute M map with given names with values of the given value types
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param name4 
 * @param name5 
 * @param valuetype1 
 * @param valuetype2 
 * @param valuetype3 
 * @param valuetype4 
 * @param valuetype5 
 * @return Mmap* a new five attribute M map with given names with values of the given value types
 */
Mmap* _getFiveArgumentMap(char* name1,char* name2,char* name3,char *name4,char *name5,Mvaluetype valuetype1,Mvaluetype valuetype2,Mvaluetype valuetype3,Mvaluetype valuetype4,Mvaluetype valuetype5){Mallocationowner owner=getOwner(__LINE__);
	if(name1!=NULL&&name2!=NULL&&name3!=NULL&&name4!=NULL&&name5!=NULL){
		if(strlen(name1)&&strlen(name2)&&strlen(name3)&&strlen(name4)&&
			strcmp(name1,name2)&&strcmp(name1,name3)&&strcmp(name1,name4)&&strcmp(name1,name5)&&
			strcmp(name2,name3)&&strcmp(name2,name4)&&strcmp(name2,name5)&&
			strcmp(name3,name4)&&strcmp(name3,name5)&&
			strcmp(name4,name5)){
			Mallocationowner owner_mapelement=Msubowner(owner,1);
			Mmapelement* _mapelement1=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
			Mmapelement* _mapelement2=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
			Mmapelement* _mapelement3=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
			Mmapelement* _mapelement4=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
			Mmapelement* _mapelement5=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner_mapelement);
			if(_mapelement1!=NULL&&_mapelement2!=NULL&&_mapelement3!=NULL&&_mapelement4!=NULL&&_mapelement5!=NULL){
				Mmap* _map=(Mmap*)CALLOC_1(sizeof(Mmap),'M',owner);
				if(_map!=NULL){
					Mallocationowner owner_variable=Msubowner(owner,2);
					_mapelement1->_variable=_getVariableWithName(name1,VT_TOKEN,true,owner_variable);
					_mapelement2->_variable=_getVariableWithName(name2,VT_TOKEN,true,owner_variable);
					_mapelement3->_variable=_getVariableWithName(name3,VT_TOKEN,true,owner_variable);
					_mapelement4->_variable=_getVariableWithName(name4,VT_TOKEN,true,owner_variable);
					_mapelement5->_variable=_getVariableWithName(name5,VT_TOKEN,true,owner_variable);
					if(_mapelement1->_variable!=NULL&&_mapelement2->_variable!=NULL&&_mapelement3->_variable!=NULL&&_mapelement4->_variable!=NULL&&_mapelement5->_variable!=NULL){
						_map->_first=_mapelement1;
						_mapelement1->_next=_mapelement2;
						_mapelement2->_next=_mapelement3;
						_mapelement3->_next=_mapelement4;
						_mapelement4->_next=_mapelement5;
						_map->_last=_mapelement5;
						_map->numberOfElements=5;
						return disowned_map(_map,owner);
					}
					FREE_MAP(_map,owner); // failed to create the five map attribute variables, so get rid of the map NOTE free_mapelement() will free the associated variable (if any)
				}
			}
			// either map element might have been created and we need to release them
			FREE_MAPELEMENT(_mapelement1,false,owner);
			FREE_MAPELEMENT(_mapelement2,false,owner);
			FREE_MAPELEMENT(_mapelement3,false,owner);
			FREE_MAPELEMENT(_mapelement4,false,owner);
			FREE_MAPELEMENT(_mapelement5,false,owner);
		}
	}
	return NULL;
}
/**
 * @brief returns a new five attribute M map with five token value attributes of the given names
 * 
 * @param name1 
 * @param name2 
 * @param name3 
 * @param name4 
 * @param name5 
 * @return Mmap* a new five attribute M map with five token value attributes of the given names
 */
Mmap* _getTokenTokenTokenTokenTokenMap(char* name1,char* name2,char* name3,char *name4,char *name5){return _getFiveArgumentMap(name1,name2,name3,name4,name5,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN,VT_TOKEN);}/* VALIDATED */
// end helper functions 

// ARRAY STUFF
#ifndef __PRODUCTION__
/**
 * @brief returns \p _array owned by \p owner_array
 * 
 * @param _array 
 * @param owner_array 
 * @return Marray* \p _array owned by \p owner_array
 */
Marray* owned_array(Marray * const _array,Mallocationowner owner_array){
	if(NULL==_array)return NULL;
	if(_array->values!=NULL)OWNED(_array->values,Msubowner(owner_array,1));
	return OWNED(_array,owner_array);
}
/**
 * @brief returns \p _array disowned by \p owner_array
 * 
 * @param _array 
 * @param owner_array 
 * @return Marray* \p _array disowned by \p owner_array
 */
Marray* disowned_array(Marray * const _array,Mallocationowner owner_array){
	if(NULL==_array)return NULL;
	if(_array->values!=NULL)DISOWNED(_array->values,owner_array);
	return DISOWNED(_array,owner_array);
}
#endif
/**
 * @brief returns a new M array
 * 
 * @param source the id of the requestee
 * @return Marray* a new M array
 */
Marray* __array(char* source){Mallocationowner owner=getOwner(__LINE__);
	Marray* _array=CALLOC_1(sizeof(Marray),'A',owner);
	if(NULL==_array)return NULL;
	return DISOWNED_ARRAY(_array,owner);
}
/**
 * @brief returns a new array with \p numberOfElements elements filled with \p fillValue
 * 
 * @param source 
 * @param numberOfElements the number of array elements
 * @param fillValue the (default) array element value
 * @return Marray* a new array
 */
Marray* _getArray(char* source,unsigned long long numberOfElements,Mvalue const * fillValue){Mallocationowner owner=getOwner(__LINE__);
	Marray* _array=owned_array(__array(source),owner);
	if(NULL==_array)return NULL;
	if(numberOfElements){ // _array->values should be initialized to accomodate the given number of values
		// I need to allocated enough room for numberOfElements Mvalue*
		_array->values=CALLOC(sizeof(Mvalue*),numberOfElements,-'a',Msubowner(owner,1));
		if(_array->values!=NULL){
			_array->numberOfElements=numberOfElements;
			if(fillValue!=NULL){
				outputValue("Fill value: ",fillValue,".\n");
				while(numberOfElements>0)assignValue(&_array->values[--numberOfElements],fillValue); // assign the fillValue to each element in the array
				_array->valuetype=fillValue->type; // the type of fillValue becomes the value type of the array
				if(fillValue->type==VT_ARRAY)
					_array->numberOfDimensionsLeft=fillValue->value._array->numberOfDimensionsLeft+1;
				output("Number of dimensions left: %i.\n",_array->numberOfDimensionsLeft);
			}
		}else
			outputError("Failed to allocate memory for storing the array elements");
	}
	return DISOWNED_ARRAY(_array,owner);
}
/**
 * @brief frees the first \p numberOfValues elements from \p values
 * @details freeing an M value pointer means assigning NULL to it, effectively decrementing the M value reference count
 * @param values 
 * @param numberOfValues 
 */
void free_values(Mvalue** values,unsigned long long numberOfValues){
	if(NULL==values)return;
	// disconnect every stored value
	unsigned long long l=0;while(l<numberOfValues)assignValue(&values[l++],NULL);
	FREE(values,numberOfValues,-'a');
}
/**
 * @brief frees M array \p _array
 * @param _array
 */
void free_array(Marray* _array/*,Mallocationowner owner*/){
	if(NULL==_array)return;
	// if we have values, we should disconnect them from their values (see free_values)
	if(_array->values!=NULL)free_values(_array->values,_array->numberOfElements);
	FREE_1(_array,'A');
}

/**
 * @brief returns a new M value wrapping M array \p _array
 * @details if \p _array is disowned and wrapping fails, \p _array is freed
 * @return a new M value wrapping M array \p _array
 */
Mvalue* _getValueOfArray(Marray* _array/*,Mallocationowner owner_list*/){//Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_array)return NULL;
	bool disowned_array=Misdisowned(_array);
	if(amVerboseDebugging())
		output("Wrapping a %s array.\n",(disowned_array?"disowned":"owned"));
	Mvalue* _value=__value(_array->weak?"weak array":"strong array");
	if(NULL==_value){
		if(disowned_array)free_array(_array);
		return NULL;
	}
	_value->value._array=(disowned_array?owned_array(_array,owner_value_data):_array); // MDH@22NOV2020: _value is to take over ownership of _list
	// if the array is disowned (so should any values in it be, so we should take over ownership)
	///////// MDH@13APR2022 doing this resulted in a bug (see _getValueOfList below which didn't have this to start with): if(disowned_array)if(_array->values)OWNED(_array->values,Msubowner(owner_value_data,1)); // TODO check this
	_value->type=VT_ARRAY;
	if(amVerboseDebugging())
		output("%s array wrapped.\n",(disowned_array?"disowned":"owned"));
	return _value;
}/* TODO VALIDATED */

/**
 * @brief returns a new M array copy from M array \p array
 * 
 * @param array 
 * @return Marray* a new M array copy from M array \p array
 */
Marray* _getArrayCopy(Marray const * const array){Mallocationowner owner=getOwner(__LINE__); // creates a 'deep' copy
	if(array!=NULL){
		register unsigned long long arrayindex=array->numberOfElements; // MDH@24NOV2020: HOORAY my first time use of 'register'
		Marray* _array=owned_array(_getArray("_getArrayCopy",arrayindex,NULL),owner);
		if(_array!=NULL){
			_array->numberOfDimensionsLeft=array->numberOfDimensionsLeft; // MDH@02MAY2023: let's copy this new field as well
			if(arrayindex>0){
				Mvalue **newvalueholder=_array->values+arrayindex,**valueholder=array->values+arrayindex;
				do assignValue(--newvalueholder,*(--valueholder));while(--arrayindex>0);
			}
			return disowned_array(_array,owner);
		}else
			outputError("Failed to create an array");
	}
	return NULL;
}
/**
 * @brief returns the new M string containing the text representation of (some) elements of M array \p _array
 * @param _array 
 * @param showAtStart the maximum number of initial elements to show
 * @param showAtEnd the maximum number of final elements to show
 * @return Mstring* the new M string containing the text representation of M array \p _array
 */
Mstring* _getArrayText(Marray const * const _array,long long showAtStart,long long showAtEnd){Mallocationowner owner=getOwner(__LINE__);
	bool showAll=(showAtStart==LLONG_MAX&&showAtEnd==LLONG_MAX); // MDH@26APR2024
	// MDH@0.1.7.14+25JUN2023: arrays should now be enclosed in square brackets (and lists in parentheses) 
	// as this is more like a tuple than a list (Python equivalent data structures)
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_VALUE));
	Mstring* result=owned_string(__string(),owner);
	if(result!=NULL){
		Mstring* p=result;
		unsigned long long l=(_array?_array->numberOfElements:0);
		if(report)
		{p=string_append_char(p,'a');p=string_append_char(p,'(');p=appendll(p,l);p=string_append_char(p,')');}
		p=string_append_char(p,'['); // switch to using p in appends
		if(l>0){
			long long firstAtEnd=l+1;if(showAtEnd<firstAtEnd)firstAtEnd-=showAtEnd;
			long long elementsNotIncluded=firstAtEnd-showAtStart-1;
			// output("First at end: %lld - elements not include: %lld.\n",firstAtEnd,elementsNotIncluded); // DEBUG
			unsigned long long arrayelementindex=0,lastarrayelementindex=(--l);
			do{
				if(elementsNotIncluded>0&&arrayelementindex>=firstAtEnd){ // the first to show at the end coming up next
					p=string_append(p,"(");
					p=appendll(p,elementsNotIncluded);
					p=string_append(p," element");
					if(elementsNotIncluded>1)p=string_append_char(p,'s');
					p=string_append(p," not displayed),");
					elementsNotIncluded=0; // for safety
				}
				///////outputChar('$');
				// increment listindex until it is equal to _listelement->index
				// MDH@11NOV2020 we use index 0 in sorting: if(_listelement->index==0)break; // VERY UNLIKELY AS field index should be monotonically increasing
				if(arrayelementindex<=showAtStart||arrayelementindex>=firstAtEnd){ // a displayable value
					if(arrayelementindex==showAtStart||arrayelementindex==firstAtEnd){
						p=appendll(p,arrayelementindex);
						p=string_append_char(p,':');
					}
					// replacing: if(amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
					Mstring* _arrayelementValueText=owned_string(_getValueText(_array->values[arrayelementindex],false,showAll),owner); // to be freed asap
					if(_arrayelementValueText!=NULL){
						p=string_append(p,string(_arrayelementValueText));
						FREE_STRING(_arrayelementValueText,owner); // release AFTER copying over
					}
					// if there's more coming write a comma
					if(arrayelementindex!=lastarrayelementindex)p=string_append_char(p,',');
				}
				// output("(%llu)",listindex); // DEBUG
			}while(p!=NULL&&(++arrayelementindex)<=lastarrayelementindex);
		}
		p=string_append_char(p,']');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(NULL==p){FREE_STRING(result,owner);result=NULL;}
	}
	return disowned_string(result,owner);	
}
// end ARRAY STUFF

// LIST STUFF
/**
 * @brief returns a new M value wrapping M list \p _list
 * @param _list
 * @return a new M value wrapping M list \p _list
 */
Mvalue* _getValueOfList(Mlist* _list/*,Mallocationowner owner_list*/){//Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_list)return NULL;
	bool disowned_list=Misdisowned(_list);
	if(amVerboseDebugging())
		output("Wrapping a %s list.\n",(disowned_list?"disowned":"owned"));
	Mvalue* _value=__value(_list->weak?"weak list":"strong list");
	if(NULL==_value){
		if(disowned_list)free_list(_list);
		return NULL;
	}
	_value->value._list=(disowned_list?owned_list(_list,owner_value_data):_list); // MDH@09JUN2020: _value is to take over ownership of _list
	_value->type=VT_LIST;
	if(amVerboseDebugging())
		output("%s list wrapped.\n",(disowned_list?"Disowned":"Owned"));
	return _value;
}/* VALIDATED */
/**
 * @brief checks whether \p _list is correct
 * 
 * @param _list 
 */
void checkList(Mlist* _list){
	if(_list!=NULL){
		long long l=_list->numberOfElements;
		if(l>0){
			Mlistelement* _listelement=_list->_first;
			if(_listelement!=NULL){
				// the number of elements in the list should match the number of counted elements
				long long listelementindex=0;
				while(true){
					if(l==0)outputError("More elements in list than accounted for");
					l--;
					if(_listelement->index<=listelementindex)output("%sList element index (%lld) below the expected list element index (%lld).\n",M_ERROR_PREFIX,_listelement->index,listelementindex);
					listelementindex=_listelement->index;
					if(NULL==_listelement->_next){
						if(_list->_last!=_listelement)outputError("Registered last list element not equal to the actual last list element");
						break;						
					}
					output("List element with index %llu OK.\n",_listelement->index);
					_listelement=_listelement->_next;
				}
				if(l>0)outputError("Less elements in list than accounted for");else 
				if(l<0)output("%s%lld more elements in list than counted.\n",M_ERROR_PREFIX,(-l));
			}else
				output("%sList with %lld elements does not have a first element!\n",M_ERROR_PREFIX,l);
		}else{
			if(_list->_first!=NULL)outputError("Empty list with first element");
			if(_list->_last!=NULL)outputError("Empty list with last element");
		}
	}
}/* VALIDATED */

// MDH@27DEC2020: for now static (as it's called from mfreadlines() only for now)
/**
 * @brief appends and returns the list element appended to \p _list owned by \p owner_list
 * 
 * @param _list 
 * @param owner_list 
 * @return Mlistelement* 
 */
static Mlistelement* getAppendedListelement(Mlist * const _list,Mallocationowner owner_list){Mallocationowner owner=getOwner(__LINE__);
	if(_list!=NULL){
		// NOTE it's a bit of a shortcut to immediately use Msubowner(owner_list,1) as owner
		//	  we can do that if nothing can go wrong in linking it to the end of _list which is (almost) certain
		Mlistelement* _appendedListelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',Msubowner(owner_list,1));
		if(_appendedListelement!=NULL){
			if(_list->_last!=NULL){ // a last element to connect to!!!!
				_appendedListelement->index=_list->_last->index;
				_list->_last->_next=_appendedListelement;
				_list->_last=_list->_last->_next;
			}else{
				_list->_first=_appendedListelement;
				_list->_last=_list->_first;
			}
			_appendedListelement->index++;
			_list->numberOfElements++;
			return _appendedListelement;
		}
	}
	return NULL;
}

// instead of returning a boolean we could return the assigned index (0 on failure)
// MDH@02JUN2019: check (and correct) prepending
// MDH@17OCT2019: passing in 0 should NOT do appending but prepending (use index len(l)+1 for appending!!!!!!)
//				OOPS we used to use 0 to force an append, so we now use M_LL_INVALID to force that!!!!
// MDH@05NOV2019: originally we returned 0 on failure and an unsigned result (leaving only 0 as possible return value), but by allowing to return negative values as well we can distinguish different types of failures
//				e.g. returning a negative value if we fail to insert the given element
//				NOTE: index==0 means prepend, index==M_LL_INVALID means append, index<0 means insert from the back (i.e. relative to the maximum index)
//				TODO: determine the situations where we want to return either M_LL_INVALID or 0 or a negative value to indicate failure
//				DOING: I suppose returning M_LL_INVALID when there's something wrong with the input, 0 when unable to comply somehow (e.g. when the list is immutable)
/**
 * @brief appends or inserts M value \p _value to M list \p _list owned by \p owner_list
 * @details if \p index is not equal to M_LL_INVALID \p value is inserted at position \p index
 * @param _list 
 * @param owner_list
 * @param _value
 * @param index
 * @return the index of the element wrapping \p _value appended or inserted, zero or negative on failure (M_LL_INVALID which is negative indicating undefined \p _list)
 */
/*unsigned*/ long long appendedToList(Mlist * const _list,Mallocationowner owner_list,Mvalue const * const _value,long long index){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_list){outputError("No list to append to");return M_LL_INVALID;} // MDH@18OCT2019: let's allow NULLing list elements (i.e. accepting _value to be NULL)
	if(_list->unlockCode>0){outputError("Unable to change the list: it is immutable");return 0;}
	// MDH@05NOV2019: let's always allow adding NULL or undefined values to a list
	if(_value!=NULL&&_value->type!=VT_UNDEFINED&&_list->valuetype!=VT_UNDEFINED)
	if(_value->type!=_list->valuetype){
		output(M_ERROR_PREFIX);
		outputValue("Unable to add '",_value,"'");
		output(" of type '%s' to a list of type '%s'.\n",VALUETYPENAMES[_value->type],VALUETYPENAMES[_list->valuetype]);
		/////return 0;
	}
	// check validity of index first
	long long lastindex=(_list->_last!=NULL?_list->_last->index:0); // ASSERT lastindex nonnegative
	// MDH@17OCT2019: index 0 now does not indicate to append to the end anymore but now indicates that the given value should be prepended!!!!
	// MDH@05NOV2019: if supposed to append the value, and the current last index is already equal to the maximum possible index, we consider the list to be full
	if(index==M_LL_INVALID){
		if(lastindex==M_LL_MAX){outputError("Unable to append to a list: it is full");return 0;};
		index=lastindex+1;
	} // MDH@17OCT2019: we need to be able to append as well (can't use 0 anymore!!!!)
	if(index<0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
	// MDH@17OCT2019: a negative index might still end up with index 0, this happens with -len(x)-1, ok, for now just accept this when it happens
	if(index<0){output("%sIndex %lld of (new) list element too small.\n",M_ERROR_PREFIX,index);return M_LL_INVALID;} // MDH@17OCT2019: can't return negative value!!! // MDH@05NOV2019: to indicate invalid input
	// if(amVerboseDebugging()){outputValue("Adding '",_value,"' to a list");output(" at index %lld.\n",index);}
	// MDH@23MAY2019: let's allow inserting or replacing as well
	// determine _listelement as element to host the value, store the successor in _nextlistelement
	Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index>0&&index<=lastindex?_list->_first:NULL);
	if(_listelement!=NULL){ // we are not appending and we have a first element, so this might be an insert or replace
		// NOTE testing _listelement is just a fail-safe as that should never happen
		while(index>_listelement->index){
			_prevListelement=_listelement;
			if(NULL==_listelement->_next){output("%sIndex (%llu) ",M_BUG_PREFIX,_listelement->index);outputValue("of existing list element '",_listelement->_value,"' probably out of order.\n");return M_LL_INVALID;}
			_listelement=_listelement->_next;
		}
		// if we're going to insert there will be a successor
		if(_listelement->index!=index){
			_nextListelement=(_prevListelement!=NULL?_prevListelement->_next:_list->_first);
			_listelement=NULL;
		} // so that we are forced to create one
	}else // we'll be insertingappending/prepending, so the current last is the predecessor (and no successor)
	if(index>0) // MdH@17OCT2019: when not prepending...
		_prevListelement=_list->_last;
	// if we do not have a list element ascertain to have one
	if(NULL==_listelement){ // not yet present in list, so we have to create a new element
		_listelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',owner);
		if(NULL==_listelement){outputError("Failed to create a list element to insert");return 0;} // failure
	}
	// MDH@02NOV2019: if the list is flagged as weak we do not (de)reference values (and copy lists and maps as assignValue() does)
	if(!_list->weak){
		assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
		///////// if(Misdisowned(_value))SUBOWNED(OWNED(_value,owner_list),2); // MDH@15MAY2020: make _listelement owned by the given list
	}else
		_listelement->_value=_value;
	// outputValue("Count of value '",_value,"' added to list:");output("%zd\n",_listelement->_value->count); // DEBUG
	// if replacing i.e. the index of _listelement matches index, we're done
	// if index equals 0 it WILL be equal to _listelement->index (which is initialized to 0 for sure)
	if(_listelement->index!=index){ // insert or append
		SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list

		// MDH@15MAR2023: Typically any MValue is owned by the global MValue list, and perhaps therefore any list referencing it can never own it
		//                if the list is weak, _value is stored in _listelement->_value as is without incrementing the value's reference count
		//                if the list is not weak, the assignment will create a copy of composite values but simple values are assigned as is with incrementing their reference count
		// DECISION: NOT taking over the ownership of Mvalue as it is owned globally and should remain owned globally as such, the list itself should never free it itself, therefore commenting the following!!!!
		/*
		// MDH@24JAN2023: OOPS Have to take over the ownership of the _value as well!!!!!! TODO check whether this is ok to do!!!!!
		if(Misowned(_listelement->_value))outputValue("List element value '",_listelement->_value,"' is not disowned!\n");else SUBOWNED(OWNED(_listelement->_value,owner_list),2); 
		*/

		_listelement->index=index;
		// linking
		if(_prevListelement!=NULL)_prevListelement->_next=_listelement;else _list->_first=_listelement;
		if(NULL==_nextListelement){if(_list->_last)_list->_last->_next=_listelement;_list->_last=_listelement;}else _listelement->_next=_nextListelement;
		(_list->numberOfElements)++; // an additional element
	}else
	if(index==0){ // prepending
		SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list
		/* MDH@15MAR2023: don't think ownership of the value should be taken over!!!!!
		// MDH@24JAN2023: OOPS Have to take over the ownership of the _value as well!!!!!! TODO check whether this is ok to do!!!!!
		if(!Misowned(_listelement->_value))SUBOWNED(OWNED(_listelement->_value,owner_list),2);else outputValue("New list element value '",_listelement->_value,"' is not disowned!\n");
		*/
		// linking into the list
		_listelement->_next=_list->_first;
		_list->_first=_listelement;
		if(NULL==_list->_last)_list->_last=_list->_first;
		(_list->numberOfElements)++;
		// if(amVerboseDebugging())outputValue("\tPrepending '",_listelement->_value,"'.\n");
		// we should increment the index of all elements (consuming _listelement on the go which is OK)
		_nextListelement=_listelement;
		while(_nextListelement!=NULL){
			// if(amVerboseDebugging())
			// {outputValue("\tIncrementing the index of '",_nextListelement->_value,"'.\n");} // DEBUG
			(_nextListelement->index)++;
			// if(amVerboseDebugging()){outputValue("\tIndex of '",_nextListelement->_value,"' incremented");output(" to %llu.\n",_nextListelement->index);} // DEBUG
			_nextListelement=_nextListelement->_next;
			// if(amVerboseDebugging()){if(_nextListelement)outputInfo("\tA next element to consider!");else outputInfo("\tNo next element to consider!");}
		}
		//if(amVerboseDebugging()){outputValue("\t'",_listelement->_value,"' prepended to a list");output(" (now) with %llu elements.\n",_list->numberOfElements);}
	}
	if(amDebugging())checkList(_list);
	///////if(amVerbose())output("New list index: %llu.\n",_listelement->index);
	return _listelement->index;
}/* VALIDATED */
// MDH@23NOV2020: inserting is similar to appending except that it should NOT replace a value but put it in front of it
/**
 * @brief inserts M value \p _value in list \p _list owned by \p owner_list at index \p index
 * 
 * @param _list 
 * @param owner_list 
 * @param _value 
 * @param index 
 * @return long long the index of the inserted element (zero or negative on failure)
 */
long long insertedIntoList(Mlist * const _list,Mallocationowner owner_list,Mvalue const * const _value,long long index){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_list){outputError("No list to insert into");return M_LL_INVALID;} // MDH@18OCT2019: let's allow NULLing list elements (i.e. accepting _value to be NULL)
	if(_list->unlockCode>0){outputError("Unable to change the list: it is immutable");return 0;}
	// MDH@05NOV2019: let's always allow adding NULL or undefined values to a list
	if(_value!=NULL&&_value->type!=VT_UNDEFINED&&_list->valuetype!=VT_UNDEFINED)
	if(_value->type!=_list->valuetype){
		output(M_ERROR_PREFIX);
		outputValue("Unable to insert '",_value,"'");
		output(" of type '%s' into a list of type '%s'.\n",VALUETYPENAMES[_value->type],VALUETYPENAMES[_list->valuetype]);
		return 0;
	}
	// check validity of index first
	long long lastindex=(_list->_last?_list->_last->index:0); // ASSERT lastindex nonnegative
	// MDH@17OCT2019: index 0 now does not indicate to append to the end anymore but now indicates that the given value should be prepended!!!!
	// MDH@05NOV2019: if supposed to append the value, and the current last index is already equal to the maximum possible index, we consider the list to be full
	if(index==M_LL_INVALID){if(lastindex==M_LL_MAX){outputError("Unable to insert into a list: it is full");return 0;};index=lastindex+1;} // MDH@17OCT2019: we need to be able to append as well (can't use 0 anymore!!!!)
	if(index<0)index+=(lastindex+1); // if index is nonpositive add lastindex+1 to it
	// MDH@17OCT2019: a negative index might still end up with index 0, this happens with -len(x)-1, ok, for now just accept this when it happens
	if(index<0){output("%sIndex %lld of (new) list element too small.\n",M_ERROR_PREFIX,index);return M_LL_INVALID;} // MDH@17OCT2019: can't return negative value!!! // MDH@05NOV2019: to indicate invalid input
	if(index==0)index=1; // MDH@23NOV2020: let's NOT allow index to be zero, the minimum possible value is 1 
	// if(amVerboseDebugging()){outputValue("Adding '",_value,"' to a list");output(" at index %lld.\n",index);}
	// MDH@23MAY2019: let's allow inserting or replacing as well
	// determine _listelement as element to host the value, store the successor in _nextlistelement
	Mlistelement *_prevListelement=NULL,*_nextListelement=NULL,*_listelement=(index>0&&index<=lastindex?_list->_first:NULL);
	if(_listelement!=NULL){ // we are not appending and we have a first element, so this might be an insert or replace
		// NOTE testing _listelement is just a fail-safe as that should never happen
		while(index>_listelement->index){
			_prevListelement=_listelement;
			if(NULL==_listelement->_next){output("%sIndex (%llu) ",M_BUG_PREFIX,_listelement->index);outputValue("of existing list element '",_listelement->_value,"' probably out of order.\n");return M_LL_INVALID;}
			_listelement=_listelement->_next;
		}
		// if we're going to insert there will be a successor
		// MDH@23NOV2020: insertedIntoList() is ALWAYS inserting even if the index values are the same, 
		// therefore removing the test: if(_listelement->index!=index)
		{_nextListelement=(_prevListelement!=NULL?_prevListelement->_next:_list->_first);_listelement=NULL;} // so that we are forced to create one
	}else // we'll be insertingappending/prepending, so the current last is the predecessor (and no successor)
	//  MDH@23NOV2020 no need to test the following as we know it will always be true: if(index>0) // MdH@17OCT2019: when not prepending...
		_prevListelement=_list->_last;
	// if we do not have a list element ascertain to have one
	// MDH@23NOV2020: _listelement will always be NULL, so no need to actually test that
	// removing: if(!_listelement){ // not yet present in list, so we have to create a new element
		_listelement=(Mlistelement*)CALLOC_1(sizeof(Mlistelement),'l',owner);
		if(NULL==_listelement){outputError("Failed to create a list element to insert");return 0;} // failure
	// removing:}
	// MDH@02NOV2019: if the list is flagged as weak we do not (de)reference values (and copy lists and maps as assignValue() does)
	if(_list->weak)_listelement->_value=_value;else assignValue(&_listelement->_value,_value); // ALWAYS assign (even when replacing)
	// outputValue("Count of value '",_value,"' added to list:");output("%zd\n",_listelement->_value->count); // DEBUG
	// if replacing i.e. the index of _listelement matches index, we're done
	// if index equals 0 it WILL be equal to _listelement->index (which is initialized to 0 for sure)
	// MDH@23NOV2020: with inserting we always insert (or append), so no need to test _listelement->index!=index
	// removing: if(_listelement->index!=index){ // insert or append
		SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list
		_listelement->index=index;
		// linking
		if(_prevListelement!=NULL)_prevListelement->_next=_listelement;else _list->_first=_listelement;
		if(NULL==_nextListelement){if(_list->_last)_list->_last->_next=_listelement;_list->_last=_listelement;}else _listelement->_next=_nextListelement;
		(_list->numberOfElements)++; // an additional element
	/* removing
	}else
	if(index==0){ // prepending
		SUBOWNED(OWNED(DISOWNED(_listelement,owner),owner_list),1); // MDH@15MAY2020: make _listelement owned by the given list
		// linking into the list
		_listelement->_next=_list->_first;
		_list->_first=_listelement;
		if(!_list->_last)_list->_last=_list->_first;
		(_list->numberOfElements)++;
		// if(amVerboseDebugging())outputValue("\tPrepending '",_listelement->_value,"'.\n");
		// we should increment the index of all elements (consuming _listelement on the go which is OK)
		_nextListelement=_listelement;
		while(_nextListelement){
			// if(amVerboseDebugging())
			// {outputValue("\tIncrementing the index of '",_nextListelement->_value,"'.\n");} // DEBUG
			(_nextListelement->index)++;
			// if(amVerboseDebugging()){outputValue("\tIndex of '",_nextListelement->_value,"' incremented");output(" to %llu.\n",_nextListelement->index);} // DEBUG
			_nextListelement=_nextListelement->_next;
			// if(amVerboseDebugging()){if(_nextListelement)outputInfo("\tA next element to consider!");else outputInfo("\tNo next element to consider!");}
		}
		//if(amVerboseDebugging()){outputValue("\t'",_listelement->_value,"' prepended to a list");output(" (now) with %llu elements.\n",_list->numberOfElements);}
	}
	*/
	// MDH@23NOV2020: if we have a nextlistelement we should increment the index until it's no longer the same, i.e. all elements are shifted one position up
	while(_nextListelement!=NULL&&_nextListelement->index==index){
		_nextListelement->index=(++index);
		_nextListelement=_nextListelement->_next;
	}
	if(amDebugging())checkList(_list);
	return _listelement->index;
}/* VALIDATED */

/**
 * @brief returns the M value stored in the element with index \p index in M list \p list
 * 
 * @param _list 
 * @param index 
 * @return Mvalue* the M value stored in the element with index \p index in M list \p list
 */
Mvalue* getValueAtIndex(Mlist* _list,long long index){
	// NOTE if index is equal to zero definitely no value there!!!
	if(_list&&index){
		if(_list->_last){
			long long maxindex=_list->_last->index;
			if(index<0)index+=(maxindex+1);
			if(index>0){
				if(index==maxindex)return _list->_last->_value;
				if(index<maxindex){
					Mlistelement* _listelement=_list->_first;
					while(_listelement){
						if(_listelement->index==index)return _listelement->_value;
						if(_listelement->index>index)break; // couldn't find it!!!
						_listelement=_listelement->_next;
					}
				}
			}
		}
	}
	return NULL;
}/* VALIDATED */
/**
 * @brief returns the value holder of the element with index \p index in M list \p _list
 * @details can be used to change the value being referenced
 * @param _list 
 * @param index 
 * @return Mvalue** the value holder of the element with index \p index in M list \p _list
 */
Mvalue** getValueHolderAtIndex(Mlist* _list,long long index){
	// NOTE if index is equal to zero definitely no value there!!!
	if(_list!=NULL&&index!=0){
		// if(amVerboseDebugging()){outputList("Determining the value holder in '",_list,"'");output(" at index %lld.\n",index);}
		if(_list->_last!=NULL){
			long long maxindex=_list->_last->index;
			if(index<0)index+=(maxindex+1);
			if(index>0){
				if(index==maxindex)return &(_list->_last->_value);
				if(index<maxindex){
					Mlistelement* _listelement=_list->_first;
					while(_listelement!=NULL){
						if(_listelement->index==index){
							// if(amVerboseDebugging())outputValue("\tFound with value '",_listelement->_value,"'.\n");
							return &(_listelement->_value);
						}
						if(_listelement->index>index)break; // couldn't find it!!!
						_listelement=_listelement->_next;
					}
				}
			}
		}
	}
	return NULL;
}/* VALIDATED */// END LIST STUFF

// MAP STUFF
// MDH@22OCT2020: useful to know if a map contains a certain attribute
/**
 * @brief returns the map element with name \p attributeName in map \p map
 * 
 * @param map 
 * @param attributeName 
 * @return * Mmapelement* the map element with name \p attributeName in map \p map
 */
Mmapelement* getMapelement(Mmap const * const map,char const * const attributeName){
	Mmapelement* mapelement=(map!=NULL&&attributeName!=NULL?map->_first:NULL); // if both map and attribute name are defined, initialize map element to the first map element
	// as long as the map element is defined, and either it does not hold a variable or the variable's name does not match the given attribute name, select the next map element
	while(mapelement!=NULL&&(!mapelement->_variable||strcmp(mapelement->_variable->_name->chars,attributeName)))mapelement=mapelement->_next;
	return mapelement;
}
// MDH@24MAY2019: if already in the map should replace the current value
/**
 * @brief appends a new attribute with name \p attributeName and value \p _attributeValue to M map \p _map owned by \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @param attributeName 
 * @param _attributeValue 
 * @return long long M_TRUE on success, M_FALSE on failure, M_LL_INVALID on invalid input 
 */
long long appendedToMap(Mmap* const _map,Mallocationowner owner_map,char const * const attributeName,Mvalue const * const _attributeValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_VALUE));
	long long result=(_map!=NULL&&attributeName!=NULL?M_FALSE:M_LL_INVALID);
	if(result!=M_LL_INVALID){
		if(!_map->unlockCode){ // the map is mutable
			// MDH@05NOV2019: let's always allow adding NULL or undefined values to a map, but otherwise the type of _attributeValue should match the type of values the map allows
			if(NULL==_attributeValue||_attributeValue->type==VT_UNDEFINED||_map->valuetype==VT_UNDEFINED||_attributeValue->type==_map->valuetype){
				if(report)
				{output("Setting the value of attribute '%s'",attributeName);outputValue(" to '",_attributeValue,"'.\n");}
				// MDH@22OCT2020: get the map element associated with the given attribute name (without creating it)
				Mmapelement* _mapelement=getMapelement(_map,attributeName);
				/* replacing:
				Mmapelement* _mapelement=_map->_first;
				while(_mapelement&&(!_mapelement->_variable||strcmp(_mapelement->_variable->_name->chars,attributeName)))_mapelement=_mapelement->_next;
				*/
				if(NULL==_mapelement){ // not found
					if(report)outputInfo("Attribute not found");
					_mapelement=(Mmapelement*)CALLOC_1(sizeof(Mmapelement),'m',owner); // NOTE no need to set _next because it is now NULL
					if(_mapelement!=NULL){
						// MDH@09JUN2020: we can immediately set the owner of _variable to be in the map because if we succeed in creating it that's where it will go
						// MDH@12MAR2020: I suppose we would like to be able to change the map property value (now using dot notation as well), so the mutable flag should be true not false
						// MDH@25MAY2020: we're disowning _variable because we 
						Mvariable* _variable=_getVariableWithName(attributeName,VT_UNDEFINED,false,Msubowner(owner_map,2)); // TODO why would this 'variable' be mutable, and allowing all values????
						if(_variable!=NULL){ // the variable was created so attach in map
							if(report)outputInfo("Map element created");
							 // pass ownership of _mapelement to _map at the first sublevel
							_mapelement->_variable=_variable; // pass ownership of _variable to the mapelement at the second sublevel in the map
							if(_map->numberOfElements)_map->_last->_next=_mapelement;else _map->_first=_mapelement;
							_map->_last=SUBOWNED(OWNED(DISOWNED(_mapelement,owner),owner_map),1);
							_map->numberOfElements++;
							// result=M_TRUE; // success
						}else{ // we have a map element BUT no variable, so no go
							FREE_MAPELEMENT(_mapelement,false,owner);_mapelement=NULL;
							outputError("Failed to create a new attribute");
						}
					}else
						outputError("Failed to create new map element");
				}
				if(_mapelement!=NULL){
					if(_map->weak)
						_mapelement->_variable->_value=_attributeValue;
					else // MDH@02NOB2019: if the map is weak assign directly!!
						assignValue(&_mapelement->_variable->_value,_attributeValue); // replace the current attribute value with the new value
					result=M_TRUE;
				}
			}else{
				output(M_ERROR_PREFIX);outputValue("Unable to add '",_attributeValue,"' to a map: it is of the wrong type.\n");
			}
		}else 
			outputError("Unable to change the map: it is immutable");
	}else
		outputError("No map or atribute name specified");
	if(report)output("Value %sappended to map.\n",(result==M_TRUE?"":"NOT "));
	return result;
}/* VALIDATED */
/**
 * @brief removes the attribute with name \p attributeName from M map \p _map with owner \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @param attributeName 
 * @return long long M_TRUE on success, M_FALSE or M_LL_INVALID on failure
 */
long long removedFromMap(Mmap* _map,Mallocationowner owner_map,char const * const attributeName){
	long long result=(_map!=NULL&&attributeName!=NULL?M_FALSE:M_LL_INVALID);
	if(result!=M_LL_INVALID){
		if(!_map->unlockCode){ // the map is mutable
			Mmapelement *mapelement=_map->_first,*previousmapelement=NULL;
			while(mapelement!=NULL&&mapelement->_variable&&strcmp(mapelement->_variable->_name->chars,attributeName)){previousmapelement=mapelement;mapelement=previousmapelement->_next;}
			if(mapelement!=NULL){ // found
				if(previousmapelement!=NULL)previousmapelement->_next=mapelement->_next;else _map->_first=mapelement->_next; // disconnect the map element
				if(mapelement->_next==NULL)_map->_last=previousmapelement;else mapelement->_next=NULL;
				_map->numberOfElements--; // one less element in the map now
				FREE_MAPELEMENT(mapelement,_map->weak,owner_map); // free (all parts of) the map element
			}
			result=M_TRUE;
		}
	}
	return result;
}/* VALIDATED */

/**
 * @brief returns the value of attribute \p attributeName in M map \p _map
 * 
 * @param _map 
 * @param attributeName 
 * @return Mvalue* the value of attribute \p attributeName in M map \p _map
 */
Mvalue* getValueOfAttribute(Mmap* _map,char* attributeName){
	// MDH@14NOV2019: there's no reason why the map couldn't have an attribute with an empty name!!!!
	if(_map&&attributeName){
		Mmapelement* _mapelement=_map->_first;
		if(_mapelement){			
			// keep looking until there is a match
			while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name->chars,attributeName))_mapelement=_mapelement->_next;
			// if there is a match return the associated value
			if(_mapelement&&_mapelement->_variable)return _mapelement->_variable->_value;
		}
	}
	return NULL;
}/* VALIDATED */
// MDH@25MAR2020: sometimes we need the value holder
/**
 * @brief returns the value holder of attribute \p attributeName of M map \p _map
 * 
 * @param _map 
 * @param attributeName 
 * @return Mvalue** the value holder of attribute \p attributeName of M map \p _map
 */
Mvalue** getValueHolderOfAttribute(Mmap* _map,char* attributeName){
	// MDH@14NOV2019: there's no reason why the map couldn't have an attribute with an empty name!!!!
	if(_map&&attributeName){
		Mmapelement* _mapelement=_map->_first;
		if(_mapelement){			
			// keep looking until there is a match
			while(_mapelement&&_mapelement->_variable&&strcmp(_mapelement->_variable->_name->chars,attributeName))_mapelement=_mapelement->_next;
			// if there is a match return the associated value
			if(_mapelement&&_mapelement->_variable)return &(_mapelement->_variable->_value);
		}
	}
	return NULL;
}/* VALIDATED */

// END MAP STUFF

/**
 * @brief returns a new M value wrapping M rational \p _rational
 * @param _rational
 * @return a new M value wrapping M rational \p _rational
 */
Mvalue* _getValueOfRational(Mrational* _rational/*,Mallocationowner owner_rational*/){
	if(!_rational)return NULL;
	Mvalue* _rationalValue=__value("rational");
	if(_rationalValue){
		_rationalValue->type=VT_RATIONAL;
		_rationalValue->value._rational=(Misdisowned(_rational)?owned_rational(_rational,owner_value_data):_rational);
	}else
	if(Misdisowned(_rational))free_rational(_rational);
	return _rationalValue;
}/* VALIDATED */

// MDH@02NOV2020: because lists can be very large, we adapt _getListText with a value telling it the maximum
//				number of elements to display from the start and at the end
// MDH@03NOV2020: showing all elements but prefixing the index when it differs from the expected list index (which is one above the last one shown)
//////////Mstring* _getValueText(Mvalue* _value); // forward prototype used in getListText() and getMapText()

/**
 * @brief returns the new M string containing the text representation of some elements of the M list \p _list
 * 
 * @param _list 
 * @param showAtStart the number of initial list elements to represent
 * @param showAtEnd the maximum number of final list elements to represent
 * @return Mstring* the new M string containing the text representation of some elements of the M list \p _list
 */
Mstring* _getListText(Mlist const * const _list,long long showAtStart,long long showAtEnd){Mallocationowner owner=getOwner(__LINE__);
	bool showAll=(showAtStart==LLONG_MAX&&showAtEnd==LLONG_MAX); // MDH@26APR2024
	// MDH@0.1.7.14+25JUN2023: lists should now be enclosed inside ( and ) instead of [ and ]
	///////output("List to output.");char c;inputCharRead(&c);
	bool report=amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_VALUE);
	Mstring* result=owned_string(__string(),owner);
	if(result!=NULL){
		Mstring* p=result;
		if(report)
		{p=string_append_char(p,'l');p=string_append_char(p,'(');p=appendll(p,_list->numberOfElements);p=string_append_char(p,')');}
		p=string_append_char(p,'('); // switch to using p in appends
		/////////size_t l=_list->numberOfElements;
		long long listelementindex=0,expectedlistindex=1; // this would be the expected list index
		Mlistelement* _listelement=(_list?_list->_first:NULL);
		Mvalue* _listelementValue;
		long long firstAtEnd=_list->numberOfElements+1;if(showAtEnd<firstAtEnd)firstAtEnd-=showAtEnd;
		long long elementsNotIncluded=firstAtEnd-showAtStart-1;
		// output("First at end: %lld - elements not include: %lld.\n",firstAtEnd,elementsNotIncluded); // DEBUG
		while(p!=NULL&&_listelement!=NULL){
			listelementindex++;
			if(elementsNotIncluded>0&&listelementindex>=firstAtEnd){ // the first to show at the end coming up next
				p=string_append(p,"(");
				p=appendll(p,elementsNotIncluded);
				p=string_append(p," element");
				if(elementsNotIncluded>1)p=string_append_char(p,'s');
				p=string_append(p," not displayed),");
				elementsNotIncluded=0; // for safety
			}
			///////outputChar('$');
			// increment listindex until it is equal to _listelement->index
			// MDH@11NOV2020 we use index 0 in sorting: if(_listelement->index==0)break; // VERY UNLIKELY AS field index should be monotonically increasing
			if(listelementindex<=showAtStart||listelementindex>=firstAtEnd){ // a displayable value
				if(_listelement==_list->_first
					||_listelement==_list->_last
					||listelementindex==showAtStart
					||listelementindex==firstAtEnd
					||expectedlistindex!=_listelement->index
					){
					p=appendll(p,_listelement->index);
					p=string_append_char(p,':');
				}
				// replacing: if(amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
				Mstring* _listelementValueText=owned_string(_getValueText(_listelement->_value,false,showAll),owner); // to be freed asap
				if(_listelementValueText!=NULL){
					p=string_append(p,string(_listelementValueText));
					FREE_STRING(_listelementValueText,owner); // release AFTER copying over
				}
				// if there's more coming write a comma
				if(_listelement->_next!=NULL)p=string_append_char(p,',');
			}
			expectedlistindex=_listelement->index+1; // expected next
			_listelement=_listelement->_next;
			// output("(%llu)",listindex); // DEBUG
			/*
			while(listindex<_listelement->index){listindex++;p=string_append_char(p,',');}
			if(amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
			// MDH@17JUN2020: because weak list values can be freed without the list knowing about it we cannot display it without possibly crashing...
			if(!_list->weak){
				//////////outputChar('.');
				_listelementValue=_listelement->_value;
				if(_listelementValue){
					Mstring* _listelementValueText=owned_string(_getValueText(_listelementValue,false),owner); // to be freed asap
					if(_listelementValueText){
						p=string_append(p,string(_listelementValueText));
						FREE_STRING(_listelementValueText,owner); // release AFTER copying over
					}
				}
			}else
			if(!amVerbose()){p=appendll(p,_listelement->index);p=string_append_char(p,':');}
			_listelement=_listelement->_next;
			*/
		}
		p=string_append_char(p,')');
		/////output("List=%s",string(p));
		// if appending failed somewhere free s
		if(NULL==p){FREE_STRING(result,owner);result=NULL;}
	}
	return disowned_string(result,owner);
}/* VALIDATED */

// MDH@02MAR2020: utility function to output a list
// MDH@02NOV2020: outputList() is typically used in debugging and we want it to show all elements
/**
 * @brief outputs M list \p list prefixed by \p prefix and suffixed by \p suffix
 * 
 * @param prefix 
 * @param list 
 * @param suffix 
 */
void outputList(char const * const prefix,Mlist const * const list,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _listText=owned_string(_getListText(list,LLONG_MAX,LLONG_MAX),owner);
	output("%s%s%s",(prefix?prefix:""),string(_listText),(suffix?suffix:""));
	FREE_STRING(_listText,owner);
}/* VALIDATED */

/**
 * @brief outputs M array \p array prefixed by \p prefix and suffixed by \p suffix
 * 
 * @param prefix 
 * @param array 
 * @param suffix 
 */
void outputArray(char const * const prefix,Marray const * const array,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _arrayText=owned_string(_getArrayText(array,LLONG_MAX,LLONG_MAX),owner);
	output("%s%s%s",(prefix?prefix:""),string(_arrayText),(suffix?suffix:""));
	FREE_STRING(_arrayText,owner);
}/* VALIDATED */

/**
 * @brief returns the new M string containing the text representation of M map \p _map
 * 
 * @param _map 
 * @param showcurlybraces 
 * @param showquotes 
 * @param showmissings 
 * @return Mstring* the new M string containing the text representation of M map \p _map
 */
Mstring* _getMapText(Mmap const * const _map,bool showcurlybraces,bool showquotes,bool showmissings){Mallocationowner owner=getOwner(__LINE__);
	Mstring* result=owned_string(__string(),owner);
	if(result!=NULL){
		Mstring* p=result;
		if(amDebugging())p=string_append_char(p,'m');
		if(_map!=NULL){
			if(showcurlybraces)p=string_append_char(p,'{');
			if(_map->numberOfElements>0){
				//////output("%s",string(p));
				Mmapelement* _mapelement=_map->_first;
				while(p!=NULL&&_mapelement!=NULL){
					//////output("%s","start");
					Mvariable* _mapVariable=_mapelement->_variable;
					if(_mapVariable!=NULL){
						// MDH@24MAY2019: surround with single quotes (for now) to indicate to the user that the attribute names are alphanumeric (even though user used integers)
						if(showquotes)p=string_append_char(p,'\'');
						p=string_append(p,_mapVariable->_name->chars);
						if(showquotes)p=string_append_char(p,'\'');
						if(showmissings||!isValueUndefined(_mapVariable->_value)){
							/////output("%s",string(p));
							p=string_append_char(p,':'); // TODO should we be using single quotes or double quotes or what???? technically it's the attribute name (without)
							/////output("%s",string(p));
							Mstring* _mapelementValueText=owned_string(_getValueText(_mapVariable->_value,false,true),owner); // free asap
							/////output("Map element: %s",string(p));
							// TODO technically NULL is also a value, so shouldn't be use the undefined value text????
							if(_mapelementValueText!=NULL){
								p=string_append(p,string(_mapelementValueText)); // append 
								FREE_STRING(_mapelementValueText,owner); // release AFTER copying over
							}
						}
					}
					_mapelement=_mapelement->_next;
					if(NULL==_mapelement)break;
					p=string_append(p,","); // only when there's a next map element to process
					////output("%s","next");
				}
			}else
			if(_map->_first!=NULL)p=string_append_char(p,'?');
			//////output("%s(%d)",string(p),string_length(p));
			if(showcurlybraces)p=string_append_char(p,'}');
		}
		//////output("%s",string(p));
		// if we failed, we have to free s here!!!
		if(NULL==p){FREE_STRING(result,owner);return NULL;}
	}
	return disowned_string(result,owner);
}/* VALIDATED */

// MDH@02MAR2020: utility function to output a map
/**
 * @brief outputs the M map \p _map prefixed by \p prefix and suffixed by \p suffix
 * 
 * @param prefix 
 * @param map 
 * @param suffix 
 */

void outputMap(char const * const prefix,Mmap const * const map,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _mapText=owned_string(_getMapText(map,true,true,true),owner);
	output("%s%s%s",(prefix?prefix:""),(_mapText?string(_mapText):""),(suffix?suffix:""));
	FREE_STRING(_mapText,owner);
}/* VALIDATED */ 

// MDH@24OCT2019: if you want the value representation or perhaps the name of a constant depends on whether name is defined
/*
static Mstring* _nullValueTextRepresentation=NULL;
Mstring* _getNullValueTextRepresentation(){if(!_nullValueTextRepresentation)_nullValueTextRepresentation=_getString(M_NULL_VALUE_TEXT_REPRESENTATION);return _getNullValueTextRepresentation;}
*/
Mmap* _getFileStatPropertyMap(Mfile const * const _file); // MDH@11MAY2024
Mmap* _getFilePropertyMap(Mfile const * const _file); // prototype

/**
 * @brief returns the new M string containing the text representation of M value \p _value
 * @details this is the function used by M to show the result of command evaluation (with \p dequoted equal to true)
 * @param _value 
 * @param dequoted 
 * @return Mstring* the new M string containing the text representation of M value \p _value
 */
Mstring* _getValueText(Mvalue const * const _value,bool dequoted,bool showAll){Mallocationowner owner=getOwner(__LINE__);
	// NOTE whatever is returned should be freed
	Mstring* valueText=NULL;
	////outputChar('.');
	if(_value!=NULL){
		////outputChar('+');
		////output("TYPE: %d\n",_value->type);
		switch(_value->type){
	  	case VT_UNDEFINED:valueText=owned_string(_getString(M_UNDEFINED_VALUE_TEXT),owner);break; // calling _getString() will create a new string every time but I think we have to do that because _getValueText() typically returns something that is freed elsewhere
			case VT_INTEGER:valueText=owned_string(_getIntegerText(_value->value._integer),owner);break;
			case VT_TIME:valueText=owned_string(_getTimeText(_value->value._time),owner);break; // MDH@08DEC2020: simply?
			case VT_BIGINTEGER:valueText=owned_string(_getBigintegerText(_value->value._biginteger),owner);break; // how many characters do we need????
  		case VT_DECIMAL:valueText=owned_string(_getDecimalText(_value->value._decimal,false),owner);break; // fixedpoint to obligatory (i.e. e-notation allowed for very big/small (positive) numbers)
			case VT_RATIONAL:valueText=owned_string(_getRationalText(_value->value._rational),owner);break;
			case VT_FLOAT:valueText=owned_string(_getFloatText(_value->value._float),owner);break;
			case VT_TEXT:
				{ // show 'binary' text escaped (because it may contain non-printable characters), NOTE binary text is now stored in a VT_BYTES Mvalue
					if(_value->value._text->presuffix=='b'||_value->value._text->presuffix=='B')
						valueText=owned_string(_getEscapedStringOfText(_value->value._text,dequoted),owner);
					else
						valueText=owned_string(_getStringOfText(_value->value._text,dequoted),owner);
				}
				break; // TODO don't dequote the text!!
			case VT_BYTES: // MDH@13JUN2024: a binary string requires escaping it, and writing it without a textprefix (as VT_TEXT uses)
			  { // prefix with b, surround by single quotes
					valueText=owned_string(_getPrintableString(_value->value._string,'\'','b'),owner);
				}
				break;
			case VT_MAP:valueText=owned_string(_getMapText(_value->value._map,true,true,true),owner);break;
			case VT_ARRAY:valueText=owned_string(_getArrayText(_value->value._array,(showAll?LLONG_MAX:M_ARRAY_ELEMENTS_AT_START),(showAll?LLONG_MAX:M_ARRAY_ELEMENTS_AT_END)),owner);break;
			case VT_LIST:valueText=owned_string(_getListText(_value->value._list,(showAll?LLONG_MAX:M_LIST_ELEMENTS_AT_START),(showAll?LLONG_MAX:M_LIST_ELEMENTS_AT_END)),owner);break;
			case VT_TOKEN:
				{ // can't just show the single token because we could have following ones
					valueText=owned_string(__string(),owner);
					if(valueText!=NULL){
						Mstring* p=valueText;
						Mtoken* token=_value->value._token;
						while(p!=NULL&&token!=NULL){
							if(token->text!=NULL)p=string_append(p,string(token->text));
							token=token->next;
						}
						if(NULL==p){FREE_STRING(valueText,owner);valueText=NULL;}
					}
					// replacing: valueText=_stringCopy(_value->value._token->text,0);
				}
				break; // we need to return a copy because that copy will be freed typically (and we do not want to free the original now do we?)
			case VT_REFERENCE:
				{
					valueText=owned_string(__string(),owner);
					if(valueText!=NULL){
						Mstring* p=valueText;
						p=string_append_char(p,M_DEREFERENCE_CHARACTER);
						if(_value->value._reference!=NULL&&_value->value._reference->variable!=NULL){ // MDH@11MAR2020: possibly a reference without a variable yet associated (typically used with reference function parameters)
							p=string_append(p,_value->value._reference->variable->_name->chars);
							// append the reference index so we know which one it is
							p=string_append_char(p,':');
							p=appendll(p,_value->value._reference->referenceindex);
						}
						/* replacing:
						if(_value->value._reference->_name)p=string_append(p,_value->value._reference->_name);
						if(_value->value._reference->_itemid){
							p=string_append_char(p,'[');
							Mstring* _valueText=_getValueText(_value->value._reference->_itemid,dequoted);
							if(_valueText){p=string_append(p,string(_valueText));FREE_STRING(_valueText);}
							p=string_append_char(p,']');
						}
						if(_value->value._reference->_value){
							p=string_append_char(p,'=');
							Mstring* _valueText=_getValueText(_value->value._reference->_value,dequoted);
							if(_valueText){p=string_append(p,string(_valueText));FREE_STRING(_valueText);}
						}
						*/
						if(NULL==p){FREE_STRING(valueText,owner);valueText=NULL;}
					}
				}
				break;
			case VT_FUNCTION:
				{
					valueText=owned_string(_getString("function"),owner);
					if(valueText!=NULL){
						Mstring* p=valueText;
						Mfunction* function=_value->value._function;
						if(function!=NULL){
							p=string_append_char(p,'(');
							if(p&&function->_parameterMap){
								Mmapelement* _parameterMapelement=function->_parameterMap->_first;
								while(p!=NULL&&_parameterMapelement!=NULL){
									if(_parameterMapelement->_variable){
										p=string_append(p,_parameterMapelement->_variable->_name->chars);
										p=string_append_char(p,':');
										Mstring* _parameterValueText=owned_string(_getValueText(_parameterMapelement->_variable->_value,false,true),owner);
										p=string_append(p,string(_parameterValueText));
										FREE_STRING(_parameterValueText,owner);
									}
									_parameterMapelement=_parameterMapelement->_next;
									if(_parameterMapelement!=NULL)p=string_append_char(p,',');
								}
							}
							p=string_append_char(p,')');
							// TODO perhaps write the body only when amVerboseDebugging()?
							Mlist* functionBodyList=(function->type==FT_USER&&function->functionunion._userfunction?function->functionunion._userfunction->_bodyCommandList:NULL);
							if(p!=NULL&&functionBodyList!=NULL){
								size_t l=string_length(p);
								Mlistelement* functionBodyListelement=functionBodyList->_first;
								if(functionBodyListelement!=NULL){
									do{
										p=string_append_char(p,',');
										Mstring* _bodyCommandValueText=owned_string(_getValueText(functionBodyListelement->_value,true,true),owner);
										if(_bodyCommandValueText!=NULL)p=string_append(p,string(_bodyCommandValueText));
										FREE_STRING(_bodyCommandValueText,owner);
										functionBodyListelement=functionBodyListelement->_next;
									}while(p!=NULL&&functionBodyListelement!=NULL);
									p=string_setchar(p,'[',l);	  
								}else
									p=string_append_char(p,'[');
								p=string_append_char(p,']');
							}
						}else
							p=string_append_char(p,'?');
						if(NULL==p){FREE_STRING(valueText,owner);valueText=NULL;}
					}
				}
				break;
			case VT_ENVIRONMENT:
				{
					valueText=owned_string(_getString("environment"),owner);
					if(valueText!=NULL){
						Mstring* p=valueText;
						Menvironment* environment=_value->value._environment;
						if(environment!=NULL){
							p=string_append_char(p,'(');
							p=string_append_char(p,'\'');
							Mstring* _environmentName=owned_string(_getEnvironmentName(environment),owner);
							if(_environmentName!=NULL){
								p=string_append(p,string(_environmentName));
								FREE_STRING(_environmentName,owner);
							}
							p=string_append_char(p,'\'');
							p=string_append_char(p,')');
						}else
							p=string_append_char(p,'?');
						if(NULL==p){FREE_STRING(valueText,owner);valueText=NULL;}
					}
				}
				break;
			case VT_FILE: // MDH@28SEP2020: get the file property map and make text of it
				{
					Mmap* _filePropertyMap=owned_map(_getFilePropertyMap(_value->value._file),owner);
					valueText=owned_string(_getMapText(_filePropertyMap,true,true,true),owner);
					FREE_MAP(_filePropertyMap,owner);
				}
			default:break;
		}
	}else // the text we use for an value that is NULL!
		valueText=owned_string(_getString(M_NULL_VALUE_TEXT),owner);
	if(_value!=NULL)if(amAssisting())if(valueText)valueText=owned_string(appendll(string_append_char(valueText,'#'),_value->count),owner); // show the reference count as well
	/////outputChar('.');
	return(valueText!=NULL?disowned_string(valueText,owner):_getUndefinedValueText());
	/* replacing:
	if(valueText)return valueText;
	////////if(amVerbose())if(valueText)output("Value text: '%s'.",string(valueText));else output("Value not represented.");
	if(!_UNDEFINED_VALUETEXT)_UNDEFINED_VALUETEXT=string_append(__string(),UNDEFINED_VALUETEXT);
	return _UNDEFINED_VALUETEXT;
	*/
}/* VALIDATED */

// MDH@13MAR2020: now returning the number of characters written
/**
 * @brief outputs the (dequoted) text representation of M value \p value prefixed by \p prefix and suffixed by \p suffix
 * 
 * @param prefix 
 * @param value 
 * @param suffix 
 * @return size_t the number of characters written
 */
size_t outputValue(char const * const prefix,Mvalue const * const value,char const * const suffix){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	if(prefix!=NULL)written=output("%s",prefix);
	if(value!=NULL){
		// output("(%s)%u",TOKENTYPE_STRING[value->type],value->type); // DEBUG
		Mstring* _valueText=owned_string(_getValueText(value,false,true),owner); // free asap
		if(_valueText!=NULL){written+=output("%s",string(_valueText));FREE_STRING(_valueText,owner);_valueText=NULL;}
	}else
		written+=outputChar('-');
	if(suffix!=NULL)written+=output("%s",suffix);
	return written;
}/* VALIDATED */

/**
 * @brief returns the integer stored in M value \p _value
 * @details returns M_LL_INVALID if \p _value cannot be converted to an integer
 * @param _value 
 * @return long long the integer stored in M value \p _value
 */
long long getValueInteger(const Mvalue* const _value){Mallocationowner owner=getOwner(__LINE__);
	// ASSERTION if _value can be converted to an integer,it should not equal invalid!!!!
	if(_value!=NULL){
		switch(_value->type){
			case VT_INTEGER:return _value->value._integer->ll;
			case VT_FLOAT:return double2long(_value->value._float->ld);
			case VT_BIGINTEGER:return biginteger2long(_value->value._biginteger);
			case VT_DECIMAL:return decimal2long(_value->value._decimal);
			case VT_TEXT:
				{
					// alternative which doesn't check for 0 explicitly I think: _strtoll(_value->value._text->_c,M_LL_INVALID);
					// we should check for 0 explicitly, which should always be represented by a single '0' character i.e. without signs!!!
					// TODO are we allowing other zero representations as well???
					if(strIsZero(_value->value._text->_c))return 0;
					long long ll=atoll(_value->value._text->_c); // invalid if zero
					if(ll!=0)return ll; // valid if non-zero
				}
				break;
			case VT_BYTES:
				break;
			case VT_TOKEN:
				{
					Mstring* tokenText=_value->value._token->text;
					if(strIsZero(string(tokenText)))return 0;
					long long ll=atoll(string(tokenText)); // invalid if zero
					if(ll!=0)return ll; // valid if non-zero
				}
				break;
			case VT_RATIONAL:
				{
					Mbiginteger* _biginteger=owned_biginteger(_rational2biginteger(_value->value._rational),owner);
					if(_biginteger!=NULL){
						long long ll=biginteger2long(_biginteger);
						FREE_BIGINTEGER(_biginteger,owner);
						return ll;
					}
				}
			case VT_REFERENCE: // TODO this might be hard
				break;
   			default:break;
		}
		// TODO sometimes reals can also represent integers!!!
	}
	return M_LL_INVALID;
}/* VALIDATED */

// MDH@09OCT2019: TODO=DONE is there a better way than through parsing?????
/**
 * @brief returns the real of the big integer \p biginteger
 * 
 * @param biginteger 
 * @return long double the real of the big integer \p biginteger
 */
long double getBigintegerLongDouble(Mbiginteger* biginteger){
	return mp_get_long_double(biginteger); // MDH@28OCT2019: definitely
	/* replacing:
	long double ldBiginteger=M_LD_NAN;
   	Mstring* _bigintegerText=(biginteger?_getBigintegerText(biginteger):NULL);
	if(_bigintegerText){ldBiginteger=_strtold(string(_bigintegerText),ldBiginteger);FREE_STRING(_bigintegerText);}
	return ldBiginteger;
	*/
}
/**
 * @brief returns the long double stored in M float \p _float
 * @details returns M_LD_NAN if \p _float is NULL
 * @param _float
 * @return long double the long double stored in M float \p _float
 */
long double getRealLongDouble(const Mfloat* const _float){return(_float!=NULL?_float->ld:M_LD_NAN);}/* VALIDATED */

/**
 * @brief returns the long double stored in M value \p _value
 * @details returns M_LD_NAN if \p _value cannot be converted to a long double
 * @param _value 
 * @return long double 
 */
long double getValueLongDouble(const Mvalue* const _value){
	// MDH@09OCT2019: a little more complicated then just getting out the real in that all scalar numeric values are convertable
	long double valueLongDouble=M_LD_NAN;
	if(_value!=NULL)
	switch(_value->type){ // all scalar types should be convertible
		case VT_INTEGER:valueLongDouble=_value->value._integer->ll;break;
		case VT_BIGINTEGER:valueLongDouble=getBigintegerLongDouble(_value->value._biginteger);break;
		case VT_FLOAT:valueLongDouble=_value->value._float->ld;break;
		case VT_DECIMAL:valueLongDouble=getDecimalLongDouble(_value->value._decimal);break;
		case VT_RATIONAL:valueLongDouble=getRationalLongDouble(_value->value._rational);break;
		case VT_TEXT:valueLongDouble=_strtold(_value->value._text->_c,M_LD_NAN);break; // MDH@28OCT2019: OOPS text is also a 'scalar' 
		case VT_TOKEN: valueLongDouble=_strtold(string_remainder(_value->value._token->text,1),M_LD_NAN);break; // MDH@28OCT2019: OOPS a token is also a 'scalar' (NOTE string_remainder simply skips the quote character)
		case VT_REFERENCE: // TODO this might be hard
			break;
		default:break;
	}
	return valueLongDouble;
}/* VALIDATED */

/**
 * @brief returns the big integer stored in M value \p _value
 * @details returns NULL if \p _value cannot be converted as a big integer
 * @param _value 
 * @return Mbiginteger* the big integer stored in M value \p _value
 */
Mbiginteger* _getValueBiginteger(Mvalue const * const _value){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_value)return NULL;
	if(_value!=NULL&&_value->type==VT_BIGINTEGER)return _value->value._biginteger;
	Mbiginteger* _resultBiginteger=NULL;
	switch(_value->type){
		case VT_INTEGER:_resultBiginteger=owned_biginteger(_getBiginteger(_value->value._integer->ll),owner);break;
		case VT_RATIONAL:_resultBiginteger=owned_biginteger(_rational2biginteger(_value->value._rational),owner);break; // TODO how can we be certain that the returned big integer is actually used? well, it should as this is _getValueBiginteger meaning you have to free it if you don't use it!!!
		case VT_FLOAT:
			{
				// this is a bit of a nuisance when the double is out of the VT_INTEGER range
				_resultBiginteger=owned_biginteger(__biginteger(),owner);
				if(_resultBiginteger!=NULL&&mp_set_longdouble(_resultBiginteger,_value->value._float->ld)!=MP_OKAY)
				{FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
				if(NULL==_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
			}
			break;
		case VT_TEXT:
			{
				_resultBiginteger=owned_biginteger(__biginteger(),owner);
				if(_resultBiginteger!=NULL&&mp_read_radix(MP_INT_POINTER(_resultBiginteger),_value->value._text->_c,10)!=MP_OKAY)
				{FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
				if(NULL==_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
			}
			break;
		case VT_TOKEN:
			{
				_resultBiginteger=owned_biginteger(__biginteger(),owner);
				if(_resultBiginteger!=NULL&&mp_read_radix(MP_INT_POINTER(_resultBiginteger),string(_value->value._token->text),10)!=MP_OKAY)
				{FREE_BIGINTEGER(_resultBiginteger,owner);_resultBiginteger=NULL;}
				if(NULL==_resultBiginteger)outputValue("ERROR: Failed to convert `",_value,"` to a big integer.\n");
			}
		case VT_REFERENCE: // TODO this may be hard
			break;
		default:break;
	}
	return disowned_biginteger(_resultBiginteger,owner);
}/* VALIDATED */

// (map) list conversions
/**
 * @brief appends M list \p _list to M map \p _map owned by \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @param _list 
 * @return true on success
 * @return false on failure
 */
bool listAppendedToMap(Mmap* const _map,Mallocationowner owner_map,const Mlist* const _list){Mallocationowner owner=getOwner(__LINE__); // appends a list to a (possibly empty) map using the indices as attribute name
	bool result=(_map!=NULL); // no map, no result!
	if(result){
		if(_list){
			Mlistelement* _listelement=_list->_first;
			while(_listelement){
				Mstring* _indexValueText=owned_string(__string(),owner);
				if(_indexValueText){ // free asap
					if(!appendll(_indexValueText,_listelement->index)||appendedToMap(_map,owner_map,string(_indexValueText),_listelement->_value)<=0){outputError("Failed to append a list element to a map");result=false;}
					FREE_STRING(_indexValueText,owner); // freeing
				}else{
					result=false;
					output("ERROR: Failed to convert list element index %llu to an attribute name.\n",_listelement->index);
				}
				if(!result)break;
				_listelement=_listelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

/**
 * @brief appends M list \p _list to the M map list \p _maplist owned by \p owner_maplist
 * 
 * @param _maplist 
 * @param owner_maplist 
 * @param _list 
 * @return true on success
 * @return false on failure
 */
bool listAppendedToMaplist(Mlist* const _maplist,Mallocationowner owner_maplist,Mlist const * const _list){Mallocationowner owner=getOwner(__LINE__);
	bool result=(_maplist&&_maplist->valuetype==VT_LIST); // the destination list should only allow for list elements
	if(result){
		if(_list!=NULL){
			Mlistelement* _listelement=_list->_first;
			while(result&&_listelement!=NULL){
				// index and value of the list element are stored in a new list!!
				Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED,false,"listAppendedToMapList"); // this will be a new value that (being managed) will be freed automatically when not bound, including the list contained by it!!
				if(NULL==_maplistelementValue){outputError("Failed to create an empty list");result=false;break;}
				Mlist* _maplistelement=_maplistelementValue->value._list;
				if(NULL==_maplistelement)result=false;else
				if(appendedToList(_maplistelement,owner,_getIntegerValue(_listelement->index),M_LL_INVALID)<=0)result=false;else
				if(appendedToList(_maplistelement,owner,_listelement->_value,M_LL_INVALID)<=0)result=false;else
				if(appendedToList(_maplist,owner_maplist,_maplistelementValue,M_LL_INVALID)<=0)result=false;
				if(!result){
					// no need to clean up because all created values will be garbage collected including their contents
					break;
				}
				/* MDH@26MAY2020 replacing: if we fail to construct the maplist element or to add it
				if(!_maplistelement||appendedToList(_maplistelement,owner_maplist,_getIntegerValue(_listelement->index),M_LL_INVALID)<=0||!appendedToList(_maplistelement,_listelement->_value,M_LL_INVALID)||!appendedToList(_maplist,_maplistelementValue,M_LL_INVALID)){
					outputError("Failed to create or populate map list element");
					result=false;
				}else
				*/
				_listelement=_listelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

/**
 * @brief appends M map list \p _maplist to M list \p _list owned by \p owner_list
 * 
 * @param _list 
 * @param owner_list 
 * @param _maplist 
 * @return true on success
 * @return false on failure
 */
bool maplistAppendedToList(Mlist* const _list,Mallocationowner owner_list,const Mlist* const _maplist){
	bool result=(_list!=NULL); // no list, no result!
	if(result){
		if(_maplist!=NULL){ // something to copy
			Mlistelement* _maplistelement=_maplist->_first;
			Mvalue* _maplistelementValue;
			while(result&&_maplistelement!=NULL){
				_maplistelementValue=_maplistelement->_value;
				// each map list element should be a list with at least two elements
				if(_maplistelementValue!=NULL&&_maplistelementValue->type==VT_LIST&&_maplistelementValue->value._list->numberOfElements>1){
					// the first element in the map list element should be a positive integer that we can use as index
					long long index=getValueInteger(_maplistelementValue->value._list->_first->_value);
					// if the index is positive AND we fail to copy the second list element over, we failed!!!
					// TODO we're appending the value as is (i.e. without copying so any change will also be present in the list)
					// DONE this is ok because values are immutable in essence
					if(index>0&&appendedToList(_list,owner_list,_maplistelementValue->value._list->_first->_next->_value,index)<=0)result=false;
				}
				_maplistelement=_maplistelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

/**
 * @brief appends M map list \p _maplist to the M map \p _map owned by \p owner_map
 * 
 * @param _map 
 * @param owner_map 
 * @param _maplist 
 * @return true on success
 * @return false on failure
 */
bool maplistAppendedToMap(Mmap * const _map,Mallocationowner owner_map,Mlist const * const _maplist){Mallocationowner owner=getOwner(__LINE__);
	bool result=(_map!=NULL);
	if(result){
		if(_maplist!=NULL){
			Mlistelement* _maplistelement=_maplist->_first;
			Mvalue* _maplistelementValue;
			while(result&&_maplistelement!=NULL){
				_maplistelementValue=_maplistelement->_value;
				// each map list element should be a list with at least two elements
				if(_maplistelementValue!=NULL&&_maplistelementValue->type==VT_LIST&&_maplistelementValue->value._list->numberOfElements>1){
					// the first element becomes the key the second element the attribute value
					// BUT I suppose composite keys (maps or lists) are not allowed
					Mvalue* _attributeNameValue=_maplistelementValue->value._list->_first->_value;
					Mstring* _attributeNameValueText=owned_string(_getValueText(_attributeNameValue,true,true),owner); // using common _getValueText to text the first element
					/* replacing:
					if(_attributeNameValue){
						if(_attributeNameValue->type==VT_INTEGER)_attributeNameValueText=_getIntegerText(_attributeNameValue->value._integer);else
						if(_attributeNameValue->type==VT_FLOAT)_attributeNameValueText=_getFloatText(_attributeNameValue->value._float);else
						if(_attributeNameValue->type==VT_TEXT)_attributeNameValueText=_getStringOfText(_attributeNameValue->value._text,true); // effectively cutting of the presuffix character TODO other solution????????
					}
					*/
					if(_attributeNameValueText!=NULL){
						// TODO again the value is appended as is, but 
						// DONE that should be OK, because values are essentially immutable!!!
						if(!string_length(_attributeNameValueText)||!appendedToMap(_map,owner_map,string(_attributeNameValueText),_maplistelementValue->value._list->_first->_next->_value)){
							outputError("Failed to append a list element to a map (using the index text as attribute name)");
							result=false;
						}
						FREE_STRING(_attributeNameValueText,owner);
						// TODO is this Ok?
						// DONE yes, because appendedToMap will _strdup the char* (i.e. string(_attributeNameValueText) )
					}
				}
				_maplistelement=_maplistelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

// map to (map) list conversions
/**
 * @brief appends M map \p _map to M list \p _list owned by \p owner_list
 * 
 * @param _list 
 * @param owner_list 
 * @param _map 
 * @return true 
 * @return false 
 */
bool mapAppendedToList(Mlist* const _list,Mallocationowner owner_list,const Mmap* const _map){
	bool result=(_list!=NULL);
	if(result){
		if(_map&&_map->_first){
			Mmapelement* _mapelement=_map->_first;
			while(result&&_mapelement){
				// only add those map elements of which the key can be converted to a positive integer
				long long index=atoll(_mapelement->_variable->_name->chars);
				if(index>0&&appendedToList(_list,owner_list,_mapelement->_variable->_value,index)<=0){
					outputError("Failed to append a map element to a list (using the integer value of the name as index)");
					result=false;
				}
				_mapelement=_mapelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

/**
 * @brief appends M map \p _map to M map list \p _maplist owned by \p owner_maplist
 * 
 * @param _maplist 
 * @param owner_maplist 
 * @param _map 
 * @return true on success
 * @return false on failure
 */
bool mapAppendedToMaplist(Mlist* const _maplist,Mallocationowner owner_maplist,const Mmap* const _map){Mallocationowner owner=getOwner(__LINE__);
	bool result=(_maplist!=NULL&&_maplist->valuetype==VT_LIST);
	if(result){
		Mallocationowner owner_maplistelement=Msubowner(owner_maplist,1);
		if(_map!=NULL&&_map->_first!=NULL){
			Mmapelement* _mapelement=_map->_first;
			while(result&&_mapelement!=NULL){
			   // index and value of the list element are stored in a new list!!
				Mvalue* _maplistelementValue=_getListValue(VT_UNDEFINED,false,"mapAppededToMapList");
				Mlist* _maplistelement=(_maplistelementValue?_maplistelementValue->value._list:NULL);
				if(_maplistelement!=NULL){
					// if we fail to construct the maplist element or to add it
					// NOTE the attribute name does not start with a quote character wich we need to call _getTextValue
					// TODO the following could be restructured I suppose
					Mstring* _attributeName=owned_string(_getString("'"),owner);
					if(_attributeName!=NULL){ // should be freed
						Mstring* p=_attributeName;
						p=string_append(p,_mapelement->_variable->_name->chars);
						if(p!=NULL){
							Mvalue* _attributeNameValue=_getTextValue(string(_attributeName));
							if(_attributeNameValue!=NULL&&appendedToList(_maplistelement,owner_maplistelement,_attributeNameValue,M_LL_INVALID)>0){
								if(appendedToList(_maplistelement,owner_maplist,_mapelement->_variable->_value,M_LL_INVALID)<=0||appendedToList(_maplist,owner_maplist,_maplistelementValue,M_LL_INVALID)<=0){
									result=false;
									outputError("Failed to append the attribute value in constructing a map list element");
								}
							}else{
								result=false;
								outputError("Failed to create or add the attribute name in constructing a map list element");
							}
						}else{
							result=false;
							outputError("Failed to construct the attribute name text in constructing a map list element");
						}
						FREE_STRING(_attributeName,owner); // freed!
					}else{
						result=false;
						outputError("Failed to create a text");
					}			   
				}else{
					result=false;
					outputError("Failed to create a map list element list");
				}
				_mapelement=_mapelement->_next;
			}
		}
	}
	return result;
}/* VALIDATED */

// DECIMAL EXTRACTION
/**
 * @brief returns the M decimal represented by M value \p value in decimal context \p mpd_context
 * 
 * @param value 
 * @param mpd_context
 * @return Mdecimal* the M decimal represented by M value \p value in decimal context \p mpd_context
 */
Mdecimal* _getValueTextDecimal(Mvalue* value,mpd_context_t const * mpd_context){Mallocationowner owner=getOwner(__LINE__);
	// MDH@09OCT2019: delegating to _getTextDecimal() is preferable over doing it ourselves
	Mdecimal* _valueTextDecimal=NULL;
	if(value!=NULL){
		if(value->type!=VT_DECIMAL){
			// MDH@14APR2022: we're in trouble if we're not using the C locale in turning the given value into a text
			char* locale=setlocale(LC_ALL,NULL);setlocale(LC_ALL,"C"); // replace the current locale
			Mstring* _valueText=owned_string(_getValueText(value,true,true),owner); // TODO will there be any brackets around a repeating part of a 
			setlocale(LC_ALL,locale);
			if(_valueText!=NULL){
				_valueTextDecimal=owned_decimal(_getTextDecimal(string(_valueText),0,mpd_context),owner);
				FREE_STRING(_valueText,owner);
			}else
				outputError("Failed to create the text trying to convert a value to a decimal");
		}else
			_valueTextDecimal=owned_decimal(_getDecimalCopy(value->value._decimal),owner); // shouldn't happen though
	}
	return disowned_decimal(_valueTextDecimal,owner);
	/* replacing:
	Mdecimal* _parsedValueDecimal=__decimal(NULL,0,0);
	if(_parsedValueDecimal){

		Mstring* _valueText=_getValueText(value,true);
		if(_valueText){
			uint32_t status=0;
			mpd_qset_string(_parsedValueDecimal->mpd,string(_valueText),get_default_mpd_context(),&status);
			FREE_STRING(_valueText);
			if((status&0xEFBF)!=0){FREE_DECIMAL(_parsedValueDecimal);_parsedValueDecimal=NULL;outputError("Failed to parse the decimal text");}
		}
	}else
		outputError("Failed to create a decimal");
	return _parsedValueDecimal;
	*/
}

// the work horse of converting any value (if possible) to a decimal
// TODO shouldn't we use this function in d()???????
/**
 * @brief returns the M decimal represented by M value \p value
 * 
 * @param value 
 * @return Mdecimal* the M decimal represented by M value \p value
 */
Mdecimal* _getValueDecimal(Mvalue* value,mpd_context_t const * mpd_context){Mallocationowner owner=getOwner(__LINE__);
	Mdecimal* _decimal=NULL;
	if(value!=NULL){
		if(value->type!=VT_LIST&&value->type!=VT_MAP){
			switch(value->type){
				case VT_DECIMAL:_decimal=owned_decimal(_getDecimalCopy(value->value._decimal),owner);break;
				case VT_INTEGER:_decimal=owned_decimal(__decimal(mpd_context,value->value._integer->ll,0),owner);break; // MDH@29AUG2019: replacing a call to _getDecimal()
				case VT_BIGINTEGER:
					{
						// NOTE we need to make a copy of the big integer because otherwise FREE_RATIONAL() below would free the big integer wrapped inside the value, which would be a terrible mistake
						// MDH@26MAY2020 but now that _getRational does not use the given numerator and denominator anymore and makes a copy if necessary we can simply pass in value->value._biginteger as is
						Mrational* _rational=_getRational(value->value._biginteger,NULL,M_LD_NAN,false); // much neater, isn't it?
						/* replacing:
						Mbiginteger* _numerator=OWNED(_getBigintegerCopy(value->value._biginteger),owner);
						Mrational* _rational=_getRational(_numerator,NULL,M_LD_NAN,false);
						FREE_BIGINTEGER(_numerator,owner);
						*/
						if(_rational!=NULL){
							_decimal=owned_decimal(_getRationalDecimal(_rational,mpd_context),owner); // TODO decimal context to use here?
							FREE_RATIONAL(_rational,owner);
						}
					}
					break;
				case VT_RATIONAL:
					_decimal=owned_decimal(_getRationalDecimal(value->value._rational,mpd_context),owner); // TODO which dc to use here?
					break;
				default:
					_decimal=owned_decimal(_getValueTextDecimal(value,mpd_context),owner); // delegates to _getValueTextDecimal() which always parses the decimal from the text representation of the value
					break;
			}
		}
	}
	return disowned_decimal(_decimal,owner);
}

/**
 * @brief returns the M decimal stored in M value \p value
 * @details returns NULL if \p value does not wrap an M decimal
 * @param value 
 * @param mpd_context
 * @return Mdecimal* the M decimal stored in M value \p value
 */
Mdecimal* getValueDecimal(Mvalue* value,mpd_context_t const * mpd_context){
	return(value?(value->type==VT_DECIMAL?value->value._decimal:_getValueDecimal(value,mpd_context)):NULL);
}
// END DECIMAL EXTRACTION

// MDH@03FEB2020: extract specific data elements wrapped in values
/**
 * @brief returns the M environment stored in M value \p value
 * @details returns NULL if \p value does not wrap a M environment
 * @param value 
 * @return Menvironment* the M environment stored in M value \p value
 */
Menvironment* getValueEnvironment(Mvalue const * const value){return(value!=NULL&&value->type==VT_ENVIRONMENT?value->value._environment:NULL);}

/**
 * @brief returns a new M list of type \p valuetype
 * 
 * @param valuetype 
 * @return Mlist* a new M list of type \p valuetype
 */
Mlist* _getListOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
	// output("Allocating list (size: %zd).\n",sizeof(Mlist));
	Mlist* _list=owned_list(__list("getListOfType"),owner);
	if(NULL==_list)return NULL;
	_list->valuetype=valuetype;
	return disowned_list(_list,owner);
}/* VALIDATED */

/**
 * @brief returns a new M map of type \p valuetype
 * 
 * @param valuetype 
 * @return Mmap* a new M map of type \p valuetype
 */
Mmap* _getMapOfType(Mvaluetype valuetype){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _map=CALLOC_1(sizeof(Mmap),'M',owner);
	if(!_map)return NULL;
	_map->valuetype=valuetype;
	return disowned_map(_map,owner);
}/* VALIDATED */

/**
 * @brief returns the \p list made weak
 * 
 * @param list 
 * @return Mlist* \p list made weak
 */
Mlist* listMadeWeak(Mlist* list){if(list!=NULL)list->weak=true;return list;}

/**
 * @brief returns \p map made weak
 * 
 * @param map 
 * @return Mmap* \p map made weak
 */
Mmap* mapMadeWeak(Mmap* map){if(map)map->weak=true;return map;}

/**
 * @brief returns the sign integer of \p integer
 * @details returns 1 if \p integer is positive, 0 if \p integer is zero and -1 if \p integer is negative
 * @param integer 
 * @return long long -1, 0 or 1
 */
long long getIntegerSign(long long integer){return(integer>0?1:(integer<0?-1:0));}
/**
 * @brief returns the sign of \p value
 * @details returns M_LL_INVALID if \p value is not a single numeric value
 * @param value 
 * @return long long the sign of \p value
 */
long long getValueSign(Mvalue const * const value){
	if(value){
		if(value->type==VT_INTEGER)return getIntegerSign(value->value._integer->ll);
		if(value->type==VT_BIGINTEGER)return getBigintegerSign(value->value._biginteger);
		if(value->type==VT_FLOAT)return getLongDoubleSign(value->value._float->ld);
		if(value->type==VT_DECIMAL)return getDecimalSign(value->value._decimal);
		if(value->type==VT_RATIONAL)return getRationalSign(value->value._rational);
	}
	return M_LL_INVALID;
}

/**
 * @brief returns the zero of the value type \p valuetype
 * 
 * @param valuetype 
 * @return Mvalue* the zero of the value type \p valuetype
 */
Mvalue* getValueZeroOfType(Mvaluetype valuetype){
	// I'm going to returns the right numeric value wrapped
	if(valuetype==VT_INTEGER)return _getIntegerValue(0);
	if(valuetype==VT_BIGINTEGER)return _getValueOfBiginteger(_getBiginteger(0));
	if(valuetype==VT_FLOAT)return _getFloatValue(0.0);
	if(valuetype==VT_DECIMAL)return _getValueOfDecimal(__decimal(NULL,0,0));
	if(valuetype==VT_RATIONAL)return _getValueOfRational(_getRational(_getBiginteger(0),NULL,M_LD_NAN,false));
	return NULL;
}

// TODO turn the result type of isValueZero() into long long
/**
 * @brief returns M_TRUE if \p value is zero, M_FALSE if \p value is not zero
 * @details returns M_LL_INVALID if value is NULL or is not a numeric single value
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValueZero(Mvalue* value){
	long long result=M_LL_INVALID;
	if(value!=NULL){
		if(amVerboseDebugging())
			outputValue("Checking whether '",value,"' is zero");
		if(value->type==VT_INTEGER)result=isIntegerZero(value->value._integer);else
		if(value->type==VT_BIGINTEGER)result=isBigintegerZero(value->value._biginteger);else
		if(value->type==VT_FLOAT)result=isFloatZero(value->value._float);else
		if(value->type==VT_DECIMAL)result=isDecimalZero(value->value._decimal);else
		if(value->type==VT_RATIONAL)result=isRationalZero(value->value._rational);
	}
	if(amVerboseDebugging())
		output(": %s.\n",(result==M_TRUE?"YES":"NO"));
	return result;
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p value equals 1, M_FALSE if \p value does not equal 1
 * @details returns M_LL_INVALID if value is NULL or is not a numeric single value
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValueOne(Mvalue* value){
	long long result=M_LL_INVALID;
	if(value!=NULL){
		if(amVerboseDebugging())
			outputValue("Checking whether '",value,"' equals one");
		if(value->type==VT_INTEGER)result=isIntegerOne(value->value._integer);else
		if(value->type==VT_BIGINTEGER)result=isBigintegerOne(value->value._biginteger);else
		if(value->type==VT_FLOAT)result=isFloatOne(value->value._float);else
		if(value->type==VT_DECIMAL)result=isDecimalOne(value->value._decimal);else
		if(value->type==VT_RATIONAL)result=isRationalOne(value->value._rational);
		if(amVerboseDebugging())
			output(": %s.\n",(result==M_TRUE?"YES":"NO"));
	}
	return result;
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p value is positive, M_FALSE if \p is not positive
 * @details returns M_LL_INVALID if \p value does not store a numeric value
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValuePositive(Mvalue* value){
	long long result=M_LL_INVALID;
	if(value){
		if(amVerboseDebugging())
			outputValue("Checking whether '",value,"' is positive");
		if(value->type==VT_INTEGER)result=isIntegerPositive(value->value._integer);else
		if(value->type==VT_BIGINTEGER)result=isBigintegerPositive(value->value._biginteger);else
		if(value->type==VT_FLOAT)result=isFloatPositive(value->value._float);else
		if(value->type==VT_DECIMAL)result=isDecimalPositive(value->value._decimal);else
		if(value->type==VT_RATIONAL)result=isRationalPositive(value->value._rational); // assuming the numerator is never NULL and the denominator is always positive
		if(amVerboseDebugging())
			output(": %s.\n",(result==M_TRUE?"YES":"NO"));
	}
	return result;
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p value is negative, M_FALSE if \p is not negative
 * @details returns M_LL_INVALID if \p value does not store a numeric value
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValueNegative(Mvalue* value){
	long long result=M_LL_INVALID;
	if(value){
		if(amVerboseDebugging())
			outputValue("Checking whether '",value,"' is negative");
		if(value->type==VT_INTEGER)result=isIntegerNegative(value->value._integer);else
		if(value->type==VT_BIGINTEGER)result=isBigintegerNegative(value->value._biginteger);else
		if(value->type==VT_FLOAT)result=isFloatNegative(value->value._float);else
		if(value->type==VT_DECIMAL)result=isDecimalNegative(value->value._decimal);else
		if(value->type==VT_RATIONAL)result=isRationalNegative(value->value._rational);
		if(amVerboseDebugging())
			output(": %s.\n",(result==M_TRUE?"YES":"NO"));
	}
	return result;
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p value is scalar, M_FALSE if \p is not scalar
 * @details if \p is NULL M_FALSE is returned
 * @param value 
 * @return long long M_TRUE or M_FALSE
 */
long long isValueScalar(Mvalue* value){
	if(value!=NULL)switch(value->type){case VT_INTEGER:case VT_BIGINTEGER:case VT_DECIMAL:case VT_RATIONAL:case VT_FLOAT:case VT_TEXT:case VT_BYTES:case VT_TOKEN:return M_TRUE;default:return M_FALSE;}
	return M_LL_INVALID; // MDH@15MAR2023: better to return M_LL_INVALID if \p value is NULL
}/* VALIDATED */

// MDH@26OCT2019: TODO perhaps a reference is invalid if it is no longer pointing somewhere??????
/**
 * @brief returns M_TRUE if \p valuereference is undefined, M_FALSE otherwise
 * 
 * @param valuereference 
 * @return long long M_TRUE if \p valuereference is undefined, M_FALSE otherwise
 */
long long isReferenceUndefined(Mvaluereference* valuereference){
	return(valuereference!=NULL?M_FALSE:M_TRUE);
}

// null test for the value to be considered NULL
/**
 * @brief returns M_TRUE if \p value holds a NULL or it's type is VT_UNDEFINED, M_FALSE otherwise
 * 
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValueNull(Mvalue* value){
	long long result=M_LL_INVALID;
	if(value!=NULL)
	switch(value->type){
		case VT_INTEGER:result=(value->value._integer!=NULL?M_FALSE:M_TRUE);break;
		case VT_BIGINTEGER:result=(value->value._biginteger!=NULL?M_FALSE:M_TRUE);break;
		case VT_DECIMAL:result=(value->value._decimal!=NULL?M_FALSE:M_TRUE);break;
		case VT_RATIONAL:result=(value->value._rational!=NULL?M_FALSE:M_TRUE);break;
		case VT_FLOAT:result=(value->value._float!=NULL?M_FALSE:M_TRUE);break;
		case VT_TEXT:result=(value->value._text!=NULL?M_FALSE:M_TRUE);break;
		case VT_BYTES:result=(value->value._string!=NULL?M_FALSE:M_TRUE);break;
		case VT_ARRAY:result=(value->value._array!=NULL?M_FALSE:M_TRUE);break;
		case VT_LIST:result=(value->value._list!=NULL?M_FALSE:M_TRUE);break;
//		case VT_MATRIX:result=(value->value._matrix!=NULL?M_FALSE:M_TRUE);break;
		case VT_MAP:result=(value->value._map!=NULL?M_FALSE:M_TRUE);break;
		case VT_TOKEN:result=(value->value._token!=NULL?M_FALSE:M_TRUE);break;
		case VT_UNDEFINED:result=M_TRUE;break;
		case VT_REFERENCE:result=(value->value._reference!=NULL?M_FALSE:M_TRUE);break;
		case VT_FUNCTION:result=(value->value._function!=NULL?M_FALSE:M_TRUE);break;
		case VT_ENVIRONMENT:result=(value->value._environment!=NULL?M_FALSE:M_TRUE);break;
		case VT_FILE:result=(value->value._file!=NULL?M_FALSE:M_TRUE);break; // MDH@28SEP2020
		case VT_TIME:result=(value->value._time!=NULL?M_FALSE:M_TRUE);break; // MDH@08DEC2020
	}
	return result;
}/* VALIDATED */

long long isArrayUndefined(Marray* array){return(array!=NULL?M_FALSE:M_TRUE);}
long long isListUndefined(Mlist* list){return(list!=NULL?M_FALSE:M_TRUE);}
////long long isMatrixUndefined(Mmatrix* matrix){return(matrix!=NULL?M_FALSE:M_TRUE);}
long long isMapUndefined(Mmap* map){return(map!=NULL?M_FALSE:M_TRUE);}

// MDH@25FEB2021
/**
 * @brief returns M_TRUE if \p propertyName is an attribute of M map \p map
 * 
 * @param map 
 * @param propertyName 
 * @return true on success
 * @return false on failure
 */
bool isMapProperty(Mmap* map,char* propertyName){
	if(map!=NULL&&propertyName!=NULL){
		Mmapelement* mapelement=map->_first;
		while(mapelement!=NULL){
			if(!strcmp(mapelement->_variable->_name->chars,propertyName))return true; // found
			mapelement=mapelement->_next;
		}
	}
	return false;
}

// MDH@18JUL2019: we consider certain non-null values as undefined, this is to fill the gap between non-null values that represent missings
//				TODO is a map or list undefined when empty???????
/**
 * @brief returns M_TRUE if \p value is undefined, M_FALSE otherwise
 * @details returns M_LL_INVALID if \p value is NULL
 * @param value 
 * @return long long M_TRUE, M_FALSE or M_LL_INVALID
 */
long long isValueUndefined(Mvalue* value){
	long long result=M_LL_INVALID;
	// values that are considered NULL are also undefined (even if value is NULL)
	if(value) // we have to check the value
	switch(value->type){
		case VT_INTEGER:result=isIntegerUndefined(value->value._integer);break; // replacing: value->value._integer->ll==M_LL_INVALID;
		case VT_FLOAT:result=isFloatUndefined(value->value._float);break; // replacing: return ldIsNaN(value->value._float->ld);
		case VT_BIGINTEGER:result=isBigintegerUndefined(value->value._biginteger);break;
		case VT_DECIMAL:result=isDecimalUndefined(value->value._decimal);break; // replacing: mpd_isnan((mpd_t*)value->value._decimal); // sames right but no idea how to set/get this // decimal points directly to mpd_t so we can cast
		case VT_RATIONAL:result=isRationalUndefined(value->value._rational);break;
		case VT_TEXT:result=isTextUndefined(value->value._text);break; ////strlen(_value->value._text->_c)==0;
		case VT_BYTES:result=isStringUndefined(value->value._string);break;
		case VT_ARRAY:result=isArrayUndefined(value->value._array);break; ////Mlen(_value)==0;
		case VT_LIST:result=isListUndefined(value->value._list);break; ////Mlen(_value)==0;
//		case VT_MATRIX:result=isMatrixUndefined(value->value._matrix);break;
		case VT_MAP:result=isMapUndefined(value->value._map);break; ////Mlen(_value)==0;
		case VT_TOKEN:result=isTokenUndefined(value->value._token);break; /////string_length(_value->value._token->text)==0;
		case VT_UNDEFINED:result=M_TRUE;break;
		case VT_REFERENCE:result=(value->value._reference?M_FALSE:M_TRUE);break;
		case VT_FUNCTION:result=(value->value._function?M_FALSE:M_TRUE);break;
		case VT_ENVIRONMENT:result=(value->value._environment?M_FALSE:M_TRUE);break;
		case VT_FILE:result=(value->value._file?M_FALSE:M_TRUE);break; // MDH@28SEP2020
		case VT_TIME:result=(value->value._time?M_FALSE:M_TRUE);break; // MDH@08DEC2020
	}
	return result;
}/* VALIDATED */

// MDH@04JUN2019: based on https://stackoverflow.com/questions/4637967/algorithm-challenge-generate-continued-fractions-for-a-float/56444882#56444882
/**
 * @brief returns all \p maxiter rational approximations to long double \p ld
 * 
 * @param ld 
 * @param maxiter 
 * @return Mlist* all \p maxiter rational approximations to long double \p ld
 */
Mlist* _getLongDoubleRationalList(long double ld,uint32_t maxiter){Mallocationowner owner=getOwner(__LINE__);
	// if iterations, you're supposed to return all iteration results
	Mlist* _iterationsList=owned_list(_getListOfType(VT_UNDEFINED),owner);
	if(_iterationsList!=NULL){ // should be freed when NOT returned!!
		Mrational* _rational=NULL; // the last (computed) rational
		if(isLongDoubleUndefined(ld)!=M_TRUE){ // not a NaN (might still be Infinity though), but Infinity has a sign too   
			long long ldSign=getLongDoubleSign(ld);
			if(ldSign!=M_ZERO){ // ld is not zero 
				bool neg=(ldSign==M_NEGATIVE);if(neg)ld=-ld; // remember if negative
				// ASSERT ld is positive 
				long long pmin1=1,pmin2=0,qmin1=0,qmin2=1;
				long long a,p,q;
				long double delta,rem=ld;
				// ascertain to execute the following at least once (so when maxiter<=1 we at least get the integer part of the rational)
				for(int i=1;i<=MAX(maxiter,1);i++){
					a=lrint(floorl(rem));
					p=a*pmin1+pmin2;
					q=a*qmin1+qmin2;
					////////printf("\nIteration #%u: %lld:  %lld/%lld", i, a, p, q);
					///////printf(" - delta: %.*Lf, rem: %.*Lf",LDBL_DIG,delta,LDBL_DIG,rem);
					///// doesn't work!!!!! if(fabsl(rem)<eps)return;
					delta=(ld*q)-p;
					////////////replacing (see above): if(fabsl(delta)<=M_LD_Q_EPS)break; // if the p and q we've got are fine, stop!!!
					Mbiginteger *_numerator=owned_biginteger(_getBiginteger(neg?-p:p),owner)
							   ,*_denominator=owned_biginteger(_getBiginteger(q),owner);
					_rational=_getRational(_numerator,_denominator,delta,false/*,true*/); // construct the intermediate result without normalizing
					FREE_BIGINTEGER(_numerator,owner);FREE_BIGINTEGER(_denominator,owner); // MDH@26MAY2020 now always!!!
					if(NULL==_rational){
						output("%sFailed to construct the rational approximation %lld/%lld.\n",M_ERROR_PREFIX,p,q);
						break;
					}
					// NOTE once we have the created big integer numerator and denominator bound in _rational we're responsible of freeing _rational when not bound
					// NOT being able to append the intermediate result to the list shouldn't be enough reason to abort, as long as we manage to add the end result
					Mvalue* _rationalValue=_getValueOfRational(disowned_rational(_rational,owner));
					if(NULL==_rationalValue){FREE_RATIONAL(_rational,owner);outputError("Failed to value wrap the intermediate rational approximation to a real");break;}
					// NOTE probably best to break if we can't append approximations!!
					// NOTE no need to free _rational even then as it is bound in _rationalValue so it will be freed anyway
					if(appendedToList(_iterationsList,owner,_rationalValue,i)<=0)
					{/*FREE_RATIONAL(_rational);*/outputError("Failed to register a rational approximation");break;}
					// if we get here success in updating the iterations list!!!!
					// if delta is now zero, we're done!!!
					if(isLongDoubleZero(delta))break; ///// MDH@07JUN2019: when a list is returned like this don't stop below the system's epsilon but only when the delta is zero!!!!
					rem-=a;
					rem=1/rem;
					// shift the lot
					pmin2=pmin1;qmin2=qmin1;
					pmin1=p;qmin1=q;
				}
				/* NO NEED FOR THIS ANYMORE NOW WE'VE PLACED THE CHECK FOR delta IS zero AFTER APPENDING THE RATIONAL
				if(ldIsZero(delta)){ // success but the final rational has not yet been appended to the list!!
					// construct the last rational (i.e. the result) from p and q
					Mbiginteger* _numerator=_getBiginteger(p),*_denominator=_getBiginteger(q); // have to be freed when not bound in _rational
					if(_numerator&&_denominator)if(!neg||mp_neg(_numerator,_numerator)==MP_OKAY)_rational=_getRational(_numerator,_denominator,delta,true,false);
					if(!_rational){FREE_BIGINTEGER(_numerator);FREE_BIGINTEGER(_denominator);} // DIY freeing if failing to construct the rational
				}
				*/
			}else{ // long double is zero, TODO should we store 0 as the delta, or just NaN???? what would be the difference??????
				Mbiginteger* _numerator=owned_biginteger(__biginteger(),owner);
				Mrational* _rational=(Mrational*)OWNED(_getRational(_numerator,NULL,M_LD_NAN,false),owner);
				FREE_BIGINTEGER(_numerator,owner); // ALWAYS!!
				Mvalue* _rationalValue=_getValueOfRational(disowned_rational(_rational,owner)); // OK free __biginteger() if failing to get that _rational
				if(NULL==_rationalValue)FREE_RATIONAL(_rational,owner);else 
				if(appendedToList(_iterationsList,owner,_rationalValue,0)<=0)
					outputError("Failed to append rational approximation to the result list"); // no need to free _rational because it's value wrapper will be garbage collected!!
			}
		}
		/* 
		// _rational should contain the 'last' computed rational
		if(_rational){
			Mvalue* _rationalValue=_getRationalValue(_rational,true);
			if(_rationalValue){
				// how about reporting backwards????
				if(appendedToList(_iterationsList,_rationalValue,0))return _iterationsList;
				// ASSERT failed to append the rational approximation to the iterations list
				// free whatever's NOT being returned NOTE the list itself will be freed below
				free_value(_rationalValue);
				outputError("Failed to append the rational of a real to the result list");
			}else // failed to wrap the rational
				outputError("Failed to store the rational approximation");
		}
		free_list(_iterationsList);
		*/
	}else
		outputError("Failed to create a list for storing the rational approximations");
	return disowned_list(_iterationsList,owner);
}/* VALIDATED */

/**
 * @brief assigns M value \p _value to M value holder \p _valueholder
 * @details on success the reference count of the assigned value is incremented by 1
 *          but if \p value holds a map, array or list \p value is copied before being assigned
 * @param _valueholder 
 * @param _value 
 */
void assignValue(Mvalue** _valueholder, Mvalue const * _value){//Mallocationowner owner=getOwner(__LINE__);
	// {outputValue("Storing '",_value,"'");output(" in %p.\n",_valueholder);} // DEBUG
	// ASSERT not a composite value (so like an end node)
	if(*_valueholder!=NULL)decrementReferenceCount(*_valueholder); // if the value holder points to something, decrement that value's reference count
	// MDH@01NOV2019: it's a leap of faith to let assignValue() create copies of composite values i.e. instead of assigning _value to the *_valueholder we assign a new map or list value
	if(_value!=NULL){
		// MDH@03NOV2020 (US president election day): we can prevent unnecesary duplications by not copying unbound values (which would be literals most likely)
		if(_value->count>0){
			// create a copy of the map or list and assign it to the value holder however it would then copy the map again, and that's not what should happen!!!
			// NOTE the new map and list value get a reference count of 1 below as soon as they are bound to the value holder (as should be the case)
			//	  wait a minute a forgot to take care of the reference count of the values in _getMapCopy() and _getListCopy(), NO no need to that if they use assignValue() to 'copy' the values
			if(_value->type==VT_MAP){
				// if(amVerbose()&&amDebugging())outputValue("Copying map ",_value,".\n");
				_value=_getValueOfMap(_getMapCopy(_value->value._map));
				// if(!_value)free_map(_mapCopy,owner);
			}else
			if(_value->type==VT_LIST){
				// if(amVerbose()&&amDebugging())outputValue("Copying list ",_value,".\n");
				_value=_getValueOfList(_getListCopy(_value->value._list));
				// if(!_value)free_list(_listCopy,owner);
			}else
			if(_value->type==VT_ARRAY){
				_value=_getValueOfArray(_getArrayCopy(_value->value._array));
			}
		}
	}
	*_valueholder=_value; // replace what's being pointed to
	if(*_valueholder!=NULL)incrementReferenceCount(*_valueholder); // increment what it's pointing to now (if not NULL)
}/* VALIDATED */

// functions
/**
 * @brief returns the new M array containing the result of applying one argument function \p oneArgumentFunction to each element of M array \p _array
 * 
 * @param _array 
 * @param oneArgumentFunction 
 * @param array_valuetype
 * @return Marray* the new M array containing the result of applying one argument function \p oneArgumentFunction to each element of M array \p _array
 */
Marray* appliedToArray(Marray* _array,OneArgumentFunction oneArgumentFunction,Mvaluetype array_valuetype){Mallocationowner owner=getOwner(__LINE__);
	Marray* _result=NULL;
	if(_array!=NULL){
		unsigned long long l=_array->numberOfElements;
		if(l>0){
			_result=owned_array(_getArray("appliedToArray",_array->numberOfElements,NULL),owner);
			if(_result==NULL)return NULL;
			_result->valuetype=array_valuetype; // MDH@04AUG2023
			do{
				l--;
				assignValue(&_result->values[l],oneArgumentFunction(_array->values[l]));
			}while(l>0);
		}
	}
	return disowned_array(_result,owner);	
}
/**
 * @brief applies the function in \p functionunion to each element of \p _array as first argument, and \p additionalArguments as additional arguments
 * @details returns NULL if \p _array is NULL or does not contain any elements, or when failing to create an array with as many elements as \p _array holds
 * @param _array 
 * @param functionunion 
 * @param numberOfAdditionalArguments 
 * @param additionalArguments 
 * @return Marray* returns the array of applying the function in \p functionunion to each element of \p _array as first argument, and \p additionalArguments as additional arguments
 */
Marray* applyFunctionToArray(Marray const * const _array,Mfunctionunion functionunion,size_t numberOfAdditionalArguments,Mvalue** additionalArguments){Mallocationowner owner=getOwner(__LINE__);
	unsigned long long l=(_array!=NULL?_array->numberOfElements:0);
	Marray* _result=(l>0?owned_array(_getArray("applyFunctionToArray",l,NULL),owner):NULL);
	if(NULL==_result)return NULL;
	do{
		l--;
		Mvalue* functionValue=NULL;
		switch(numberOfAdditionalArguments){
			case 0:functionValue=functionunion.oneArgumentFunction(_array->values[l]);break;
			case 1:functionValue=functionunion.twoArgumentFunction(_array->values[l],additionalArguments[0]);break;
			case 2:functionValue=functionunion.threeArgumentFunction(_array->values[l],additionalArguments[0],additionalArguments[1]);break;
			case 3:functionValue=functionunion.fourArgumentFunction(_array->values[l],additionalArguments[0],additionalArguments[1],additionalArguments[2]);break;
			case 4:functionValue=functionunion.fiveArgumentFunction(_array->values[l],additionalArguments[0],additionalArguments[1],additionalArguments[2],additionalArguments[3]);break;
		}
		assignValue(&_result->values[l],functionValue);
	}while(l>0);
	return disowned_array(_result,owner);
}

// helpers
/**
 * @brief 
 * 
 * @param _list 
 * @param functionunion 
 * @param numberOfAdditionalArguments 
 * @param additionalArguments 
 * @return Mlist* 
 */
Mlist* applyFunctionToList(Mlist const * const _list,Mfunctionunion functionunion,size_t numberOfAdditionalArguments,Mvalue** additionalArguments){Mallocationowner owner=getOwner(__LINE__);
	unsigned long long l=(_list!=NULL?_list->numberOfElements:0);
	Mlist* _result=(l>0?owned_list(_getListOfType(VT_UNDEFINED),owner):NULL);
	if(NULL==_result)return NULL;
	Mlistelement* _listelement=_list->_first;
	while(_listelement!=NULL){
		Mvalue* functionValue=NULL;
		switch(numberOfAdditionalArguments){
			case 0:functionValue=functionunion.oneArgumentFunction(_listelement->_value);break;
			case 1:functionValue=functionunion.twoArgumentFunction(_listelement->_value,additionalArguments[0]);break;
			case 2:functionValue=functionunion.threeArgumentFunction(_listelement->_value,additionalArguments[0],additionalArguments[1]);break;
			case 3:functionValue=functionunion.fourArgumentFunction(_listelement->_value,additionalArguments[0],additionalArguments[1],additionalArguments[2]);break;
			case 4:functionValue=functionunion.fiveArgumentFunction(_listelement->_value,additionalArguments[0],additionalArguments[1],additionalArguments[2],additionalArguments[3]);break;
		}
		if(appendedToList(_result,owner,functionValue,_listelement->index)<=0)break; // TODO add error message here!!!
		_listelement=_listelement->_next;
	}
	return disowned_list(_result,owner);
}
// TODO replacing
/**
 * @brief returns the new M list containing the result of applying one argument function \p oneArgumentFunction to each element of M list \p _list
 * 
 * @param _list
 * @param oneArgumentFunction 
 * @param list_valuetype the value type to assign to the returned list
 * @return Mlist* the new M list containing the result of applying one argument function \p oneArgumentFunction to each element of M list \p _list
 */
Mlist* appliedToList(Mlist* _list,OneArgumentFunction oneArgumentFunction,Mvaluetype list_valuetype){Mallocationowner owner=getOwner(__LINE__);
	Mlist* _result=NULL;
	if(_list!=NULL){
		_result=owned_list(_getListOfType(list_valuetype),owner);
		if(NULL==_result)return NULL;
		Mlistelement* _listelement=_list->_first;
		while(_listelement!=NULL&&appendedToList(_result,owner,oneArgumentFunction(_listelement->_value),_listelement->index)>0)_listelement=_listelement->_next;
	}
	return disowned_list(_result,owner);
}/* VALIDATED */

/**
 * @brief 
 * 
 * @param _map 
 * @param functionunion 
 * @param numberOfAdditionalArguments 
 * @param additionalArguments 
 * @return Mmap* 
 */
Mmap* applyFunctionToMap(Mmap const * const _map,Mfunctionunion functionunion,size_t numberOfAdditionalArguments,Mvalue** additionalArguments){Mallocationowner owner=getOwner(__LINE__);
	unsigned long long l=(_map!=NULL?_map->numberOfElements:0);
	Mmap* _result=(l>0?owned_map(_getMapOfType(VT_UNDEFINED),owner):NULL);
	if(NULL==_result)return NULL;
	Mmapelement* _mapelement=_map->_first;
	while(_mapelement!=NULL){
		Mvalue* functionValue=NULL;
		switch(numberOfAdditionalArguments){
			case 0:functionValue=functionunion.oneArgumentFunction(_mapelement->_variable->_value);break;
			case 1:functionValue=functionunion.twoArgumentFunction(_mapelement->_variable->_value,additionalArguments[0]);break;
			case 2:functionValue=functionunion.threeArgumentFunction(_mapelement->_variable->_value,additionalArguments[0],additionalArguments[1]);break;
			case 3:functionValue=functionunion.fourArgumentFunction(_mapelement->_variable->_value,additionalArguments[0],additionalArguments[1],additionalArguments[2]);break;
			case 4:functionValue=functionunion.fiveArgumentFunction(_mapelement->_variable->_value,additionalArguments[0],additionalArguments[1],additionalArguments[2],additionalArguments[3]);break;
		}
		if(appendedToMap(_result,owner,_mapelement->_variable->_name->chars,functionValue)!=1)break; // TODO add an error message, and/or do not break
		_mapelement=_mapelement->_next;
	}
	return disowned_map(_result,owner);
}
// TODO replacing
/**
 * @brief returns the new M map containing the result of applying one argument function \p oneArgumentFunction to each attribute value of M map \p _map
 * 
 * @param _map
 * @param oneArgumentFunction 
 * @param map_valuetype the value type to assign to the returned map
 * @return Mmap* the new M map containing the result of applying one argument function \p oneArgumentFunction to each attribute value of M map \p _map
 */
Mmap* appliedToMap(Mmap* _map,OneArgumentFunction oneArgumentFunction,Mvaluetype map_valuetype){Mallocationowner owner=getOwner(__LINE__);
	Mmap* _result=NULL;
	if(_map!=NULL){
		_result=owned_map(_getMapOfType(map_valuetype),owner);
		if(NULL==_result)return NULL;
		Mmapelement* _mapelement=_map->_first;
		while(_mapelement!=NULL&&appendedToMap(_result,owner,_mapelement->_variable->_name->chars,oneArgumentFunction(_mapelement->_variable->_value))==1)_mapelement=_mapelement->_next;
	}
	return disowned_map(_result,owner);
}/* VALIDATED */

/**
 * @brief returns the new M big integer of the rounded value of M rational \p _rational
 * 
 * @param _rational 
 * @return Mbiginteger* the new M big integer of the rounded value of M rational \p _rational
 */
Mbiginteger* _getRoundedRationalInteger(Mrational* _rational){Mallocationowner owner=getOwner(__LINE__);
	// we have to multiply the numerator by 2 and add the denominator
	// NO we should compare the remainder with the denominator divided by two
	// BUT if we multiply the numerator with 2 we can divide and compare with 
	// TODO ignores delta for now
	if(_rational!=NULL){
		// denominator equal to 1?
		if(NULL==_rational->den||mp_cmp(MP_INT_POINTER(_rational->den),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return disowned_biginteger(owned_biginteger(_rational->num?_getBigintegerCopy(_rational->num):_getBiginteger(1),owner),owner);
		// if the numerator equals 1, the result is 0
		if(NULL==_rational->num||mp_cmp(MP_INT_POINTER(_rational->num),MP_INT_POINTER(getBigintegerOne()))==MP_EQ)return disowned_biginteger(owned_biginteger(_getBiginteger(0),owner),owner); // with the numerator at least equal to 2 the result will always be 0
		bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
		// get the absolute value of the numerator
		Mbiginteger* _dividend=NULL;
		Mbiginteger* _absnum=owned_biginteger(__biginteger(),owner); // to be freed asap
		if(_absnum!=NULL){ // freeable
			if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))==MP_OKAY){
				Mbiginteger* _twicenum=owned_biginteger(__biginteger(),owner);
				if(_twicenum!=NULL){ // freeable
					if(mp_mul_2(MP_INT_POINTER(_absnum),MP_INT_POINTER(_twicenum))==MP_OKAY){
						Mbiginteger* _twiceden=owned_biginteger(__biginteger(),owner);
						if(_twiceden!=NULL){
							if(mp_mul_2(MP_INT_POINTER(_rational->den),MP_INT_POINTER(_twiceden))==MP_OKAY){
								Mbiginteger* _remainder=owned_biginteger(__biginteger(),owner);
								if(_remainder!=NULL){
									_dividend=owned_biginteger(__biginteger(),owner);
									if(_dividend!=NULL){
										outputBiginteger("Integer dividing ",_twicenum,NULL);outputBiginteger(" by ",_twiceden,".\n");
										bool success=(mp_div(MP_INT_POINTER(_twicenum),MP_INT_POINTER(_twiceden),MP_INT_POINTER(_dividend),MP_INT_POINTER(_remainder))==MP_OKAY);
										// increment _dividend if _remainder larger than denominator
										if(success){
											outputBiginteger("Integer dividing ",_twicenum,NULL);outputBiginteger(" by ",_twiceden,"=");outputBiginteger(NULL,_dividend,NULL);outputBiginteger(":",_remainder,".\n");
											if(mp_cmp(MP_INT_POINTER(_remainder),MP_INT_POINTER(_rational->den))==MP_GT&&mp_incr(MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
											if(neg&&mp_neg(MP_INT_POINTER(_dividend),MP_INT_POINTER(_dividend))!=MP_OKAY)success=false;
										}
										if(!success){FREE_BIGINTEGER(_dividend,owner);_dividend=NULL;}
									}else 
										outputError("Failed to create big integer dividend");
									FREE_BIGINTEGER(_remainder,owner);
								}else 
									outputError("Failed to create big integer remainder");
							}else
								outputError("Failed to double big integer denominator");
							FREE_BIGINTEGER(_twiceden,owner);
						}
					}else
						outputError("Failed to double big integger numerator");
					FREE_BIGINTEGER(_twicenum,owner); // freed!
				}else
					outputError("Failed to create big integer");
			}else
				outputError("Failed to compute the absolute of a big integer");
			FREE_BIGINTEGER(_absnum,owner); // freed!
		}
		return disowned_biginteger(_dividend,owner);
	}
	return NULL;
}/* VALIDATED */

/*
 * \parameter floor  when true, returns the integer part of the rational, which actually is the trunc value
 * \parameter towardszero is true, returns the integer part of the rational, which actually is the trunc version
 */

/**
 * @brief returns the new M big integer 'near' to M rational \p _rational
 * @details if \p floor is set, truncates the rational, rounds otherwise
 *          if \p towardszero is set, truncates towards zero, truncates to minus infinity otherwise
 * @param _rational 
 * @param floor 
 * @param towardszero 
 * @return Mbiginteger* the new M big integer 'near' to M rational \p _rational
 */
Mbiginteger* _getRationalInteger(Mrational* _rational,bool floor,bool towardszero){Mallocationowner owner=getOwner(__LINE__);
	// TODO ignores delta for now
	if(_rational!=NULL){
		// TODO if the numerator is NULL which is possible now, we can still round
		if(isBigintegerOne(_rational->den)!=0)return _getBigintegerCopy(_rational->num); // if the denominator equals 1, return the numerator...
		if(NULL==_rational->num){
			// the denominator is at least two
			return _getBiginteger(0); // TODO check if this is correct!!!!!
		}
		// ASSERT both numerator and denominator are NOT NULL
		bool neg=mp_isneg(MP_INT_POINTER(_rational->num)); // determine whether negative or not
		// get the absolute value of the numerator
		Mbiginteger* _absnum=owned_biginteger(__biginteger(),owner); // to be freed asap
		if(mp_abs(MP_INT_POINTER(_rational->num),MP_INT_POINTER(_absnum))!=MP_OKAY)
		{FREE_BIGINTEGER(_absnum,owner);outputError("Failed to compute the absolute of a big integer");return NULL;}
		Mbiginteger *_dividend=owned_biginteger(__biginteger(),owner),*_remainder=owned_biginteger(__biginteger(),owner);
		bool success=(mp_div(MP_INT_POINTER(_absnum),MP_INT_POINTER(_rational->den),MP_INT_POINTER(_dividend),MP_INT_POINTER(_remainder))==MP_OKAY);
		if(success&&mp_iszero(MP_INT_POINTER(_remainder))!=MP_YES){ // division succeeded with a non-zero remainder
			if(neg){
				if(mp_neg(MP_INT_POINTER(_dividend),MP_INT_POINTER(_dividend))==MP_OKAY){
					// if flooring (instead of ceiling) we have to subtract one
					if(floor&&!towardszero&&mp_decr(MP_INT_POINTER(_dividend))!=MP_OKAY){
						success=false;
						outputError("Failed to decrement the truncated negative big integer");
					}
				}else{
					success=false;
					outputError("Failed to negate the big integer dividend");
				}
			}else{
				if(!floor&&!towardszero&&mp_incr(MP_INT_POINTER(_dividend))!=MP_OKAY){
					success=false;
					outputError("Failed to increment truncated positive big integer");
				}
			}
			if(!success){FREE_BIGINTEGER(_dividend,owner);_dividend=NULL;}
		}
		FREE_BIGINTEGER(_remainder,owner);
		FREE_BIGINTEGER(_absnum,owner);
		return disowned_biginteger(_dividend,owner);
	}
	return NULL;
}

// MDH@18OCT2019: same for decimals
// TODO should we store the result in a big integer or an integer (if possible????)
//	  theoretically we should return a decimal!!!
/**
 * @brief returns the M decimal integer 'nearest' to M decimal \p _decimal
 * 
 * @param _decimal 
 * @param floor 
 * @param towardszero 
 * @return Mdecimal* the M decimal integer 'nearest' to M decimal \p _decimal
 */
Mdecimal* _getDecimalInteger(Mdecimal* _decimal,bool floor,bool towardszero){Mallocationowner owner=getOwner(__LINE__);
	// NOTE decimals can have repeating parts BUT those repeating digits are only behind the decimal comma, so won't have effect on the integers
	// ceil: false,false / trunc: true,true / floor: true,false / ?: false,true
	if(_decimal!=NULL){
		Mdecimalcontext* decimalcontext=getDecimalcontext(_decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext!=NULL?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
		if(mpd_context!=NULL){
			if(towardszero){ // always means truncing!!!
				Mdecimal* _truncDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
				if(_truncDecimal!=NULL){
					uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
					mpd_qtrunc(_truncDecimal->mpd,_decimal->mpd,mpd_context,&status);
					if((status&0xEFBF)==0)return disowned_decimal(_truncDecimal,owner);
					output("%s",M_ERROR_PREFIX);outputDecimal("Failed to truncate decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
					FREE_DECIMAL(_truncDecimal,owner);
				}
			}else
			if(floor){
				Mdecimal* _floorDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
				if(_floorDecimal!=NULL){
					uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
					mpd_qfloor(_floorDecimal->mpd,_decimal->mpd,mpd_context,&status);
					if((status&0xEFBF)==0)return disowned_decimal(_floorDecimal,owner);
					output("%s",M_ERROR_PREFIX);outputDecimal("Failed to floor decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
					FREE_DECIMAL(_floorDecimal,owner);
				}					
			}else{
				Mdecimal* _ceilDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
				if(_ceilDecimal!=NULL){
					uint32_t status=0; // OOPS initializing to 0 absolute necessary!!!
					mpd_qceil(_ceilDecimal->mpd,_decimal->mpd,mpd_context,&status);
					if((status&0xEFBF)==0)return disowned_decimal(_ceilDecimal,owner);
					output("%s",M_ERROR_PREFIX);outputDecimal("Failed to ceil decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
					FREE_DECIMAL(_ceilDecimal,owner);
				}
			}
		}else
			outputError("No context available for converting a decimal to an integer");
	}
	return NULL;
}

/**
 * @brief returns the new M decimal of \p _decimal rounded to the nearest integer
 * 
 * @param _decimal 
 * @return Mdecimal* the new M decimal of \p _decimal rounded to the nearest integer
 */
Mdecimal* _getRoundedDecimal(Mdecimal* _decimal){Mallocationowner owner=getOwner(__LINE__);
	if(_decimal!=NULL){
		Mdecimalcontext* decimalcontext=getDecimalcontext(_decimal->prec);
		mpd_context_t* mpd_context=(decimalcontext!=NULL?decimalcontext->mpd_context:M_DECIMALCONTEXT->mpd_context);
		if(mpd_context!=NULL){
			Mdecimal* _roundDecimal=owned_decimal(__decimal(mpd_context,0,0),owner);
			if(_roundDecimal!=NULL){
				uint32_t status=0;
				mpd_qround_to_int(_roundDecimal->mpd,_decimal->mpd,mpd_context,&status);
				if((status&0xEFBF)==0)return disowned_decimal(_roundDecimal,owner);
				output("%s",M_ERROR_PREFIX);outputDecimal("Failed to round decimal '",_decimal,"'");output(" (status: %.8x).\n",status);
				FREE_DECIMAL(_roundDecimal,owner);
			}
		}else
			outputError("No context available for rounding a decimal");
	}
	return NULL;
}

// MDH@24OCT2019: when representing variable values the value might match the value of a constant in which case we use the name of that constant variable instead (which is like a symbol)
//				obviously the type of the value should match as well
/**
 * @brief returns true if the M values \p value1 and \p value2 are equal
 * 
 * @param value1 
 * @param value2 
 * @return true when \p value1 equals \p value2
 * @return false when \p value1 does not equal \p value2
 */
bool areValuesEqual(Mvalue const * const value1,Mvalue const * const value2){
	if(NULL==value1&&NULL==value2)return false; // if both NULL not considered to be the same TODO is this correct????
	if(value1==value2)return true; // the same value pointed to
	if(NULL==value1||NULL==value2)return false; // if either is NULL, not the same of course
	// ASSERT both not NULL
	if(value1->type==value2->type) // if the types are different definitely not the same
	switch(value1->type){
		case VT_INTEGER:return(value1->value._integer->ll==value2->value._integer->ll);
		case VT_FLOAT:return(areFloatsEqual(value1->value._float,value2->value._float)); // MDH@25OCT2019: replacing ldEqual() with a call to areFloatsEqual()
		case VT_BIGINTEGER:return(mp_cmp(MP_INT_POINTER(value1->value._biginteger),MP_INT_POINTER(value2->value._biginteger))==MP_EQ);
		case VT_TEXT:return(value1->value._text->presuffix==value2->value._text->presuffix&&strcmp(value1->value._text->_c,value2->value._text->_c)==0);
		case VT_BYTES:return string_equal(value1->value._string,value2->value._string); // MDH@14JUN2024
		case VT_TOKEN:return string_equal(value1->value._token->text,value2->value._token->text);
//		case VT_MATRIX:break;
		case VT_ARRAY:case VT_LIST:case VT_MAP:break;
		case VT_DECIMAL:case VT_RATIONAL:break;
		case VT_UNDEFINED:return true; // there's only ONE undefined value around??????
		case VT_REFERENCE: // TODO this might be hard
			break;
		case VT_FUNCTION:return(value1->value._function==value2->value._function);
		case VT_ENVIRONMENT:return(strcmp(value1->value._environment->_name->chars,value2->value._environment->_name->chars)==0); // TODO we might need to use the full name of the environment here though
		case VT_FILE:return string_equal(value1->value._file->_name,value2->value._file->_name); // MDH@28SEP2020: the same if files are the same (TODO should be canonical of course)
		case VT_TIME:return value1->value._time->t==value2->value._time->t;
	}
	return false;
}

// MDH@03MAR2020: moved over from Menvironment.c
/**
 * @brief frees \p _expressionlistelement
 * 
 * @param _expressionlistelement 
 */
void free_expressionlistelement(Mexpressionlistelement* _expressionlistelement){
	if(_expressionlistelement!=NULL){
		free_expressionlistelement(_expressionlistelement->_next);
		free_value(_expressionlistelement->_value);
		free(_expressionlistelement);
	}
}/* VALIDATED */
/**
 * @brief frees \p _expressionlist
 * 
 * @param _expressionlist 
 */
void free_expressionlist(Mexpressionlist* _expressionlist){
	if(_expressionlist){
		free_expressionlistelement(_expressionlist->_next);
		free_value(_expressionlist->_value);
		free(_expressionlist);
	}
}/* VALIDATED */

// NOTE typically you're not supposed to free internal functions safe M function definitions 
// TODO if a function map is freed, we shouldn't free internal functions BUT those are only present in the main environment which is never released!!
#ifndef __PRODUCTION__
/**
 * @brief returns \p _functions owned by \p owner_function
 * 
 * @param _function 
 * @param owner_function 
 * @return Mfunction* \p _functions owned by \p owner_function
 */
Mfunction* owned_function(Mfunction* _function,Mallocationowner owner_function){
	if(NULL==_function)return NULL;
	/////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
	if(_function->_parameterMap!=NULL)owned_map(_function->_parameterMap,Msubowner(owner_function,1)); // MDH@22OCT2020: with a parameter map possibly missing, testing might help
	if(_function->type==FT_USER)owned_userfunction(_function->functionunion._userfunction,Msubowner(owner_function,1)); // TODO ?????
	return OWNED(_function,owner_function);
}
/**
 * @brief returns \p _function disowned by \p owner_function
 * 
 * @param _function 
 * @param owner_function 
 * @return Mfunction* \p _function disowned by \p owner_function
 */
Mfunction* disowned_function(Mfunction* _function,Mallocationowner owner_function){
	if(NULL==_function)return NULL;
	/////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
	if(_function->_parameterMap!=NULL)disowned_map(_function->_parameterMap,owner_function);
	//if(amVerboseDebugging())output("\t\tFunction parameter map disowned.\n");
	if(_function->type==FT_USER){
		disowned_userfunction(_function->functionunion._userfunction,owner_function); // TODO ?????
		//if(amVerboseDebugging())output("\t\tUser function disowned!\n");
	}
	return DISOWNED(_function,owner_function);
}
#endif
/**
 * @brief frees M function \p _function
 * @param _function
 */
void free_function(Mfunction* _function/*,Mallocationowner owner_function*/){
	if(!_function)return;
		/////// MDH@10JUL2019: moved over to the map element containing the function! FREE_STRING(_function->_name);
	if(_function->_parameterMap)free_map(_function->_parameterMap);
	if(_function->type==FT_USER)free_userfunction(_function->functionunion._userfunction);
	FREE_1(_function,'F');
}/* VALIDATED */

#ifndef __PRODUCTION__
/**
 * @brief returns \p _functionmapelement owned by \p owner_functionmapelement
 * 
 * @param _functionmapelement 
 * @param owner_functionmapelement 
 * @return Mfunctionmapelement* \p _functionmapelement owned by \p owner_functionmapelement
 */
Mfunctionmapelement* owned_functionmapelement(Mfunctionmapelement* _functionmapelement,Mallocationowner owner_functionmapelement){
	if(!_functionmapelement)return NULL;
	owned_functionmapelement(_functionmapelement->_next,owner_functionmapelement);
	owned_function(_functionmapelement->_function,Msubowner(owner_functionmapelement,1));
	owned_string(_functionmapelement->_name,Msubowner(owner_functionmapelement,1));
	return OWNED(_functionmapelement,owner_functionmapelement);
}
/**
 * @brief returns \p _functionmapelement disowned by \p owner_functionmapelement
 * 
 * @param _functionmapelement 
 * @param owner_functionmapelement 
 * @return Mfunctionmapelement* \p _functionmapelement disowned by \p owner_functionmapelement
 */
Mfunctionmapelement* disowned_functionmapelement(Mfunctionmapelement* _functionmapelement,Mallocationowner owner_functionmapelement){
	if(!_functionmapelement)return NULL;
	disowned_functionmapelement(_functionmapelement->_next,owner_functionmapelement);
	////////output("Disowning function map element '%s'.\n",string(_functionmapelement->_name));
	////////output("\tDisowning the function.\n");
	disowned_function(_functionmapelement->_function,owner_functionmapelement);
	////////output("\tDisowning the function name.\n");
	disowned_string(_functionmapelement->_name,owner_functionmapelement);
	////////output("\tDisowning the function map element!\n");
	return DISOWNED(_functionmapelement,owner_functionmapelement);
}
#endif
/**
 * @brief frees \p _functionmapelement
 * 
 * @param _functionmapelement 
 */
void free_functionmapelement(Mfunctionmapelement* _functionmapelement){
	if(!_functionmapelement)return;
	free_functionmapelement(_functionmapelement->_next);
	free_function(_functionmapelement->_function);
	free_string(_functionmapelement->_name);
	FREE_1(_functionmapelement,'f');
}// VALIDATED
#define FREE_FUNCTIONMAPELEMENT(_functionmapelement,owner_functionmapelement) free_functionmapelement(disowned_functionmapelement(_functionmapelement,owner_functionmapelement))

#ifndef __PRODUCTION__
/**
 * @brief returns \p _functionmap owned by \p owner_functionmap
 * 
 * @param _functionmap 
 * @param owner_functionmap 
 * @return Mfunctionmap* \p _functionmap owned by \p owner_functionmap
 */
Mfunctionmap* owned_functionmap(Mfunctionmap* _functionmap,Mallocationowner owner_functionmap){
	if(!_functionmap)return NULL;
	owned_functionmapelement(_functionmap->_first,Msubowner(owner_functionmap,1));
	return OWNED(_functionmap,owner_functionmap);
}
Mfunctionmap* disowned_functionmap(Mfunctionmap* _functionmap,Mallocationowner owner_functionmap){
	if(!_functionmap)return NULL;
	disowned_functionmapelement(_functionmap->_first,owner_functionmap);
	return DISOWNED(_functionmap,owner_functionmap);
}
#endif
/**
 * @brief frees \p _functionmap
 * 
 * @param _functionmap 
 */
void free_functionmap(Mfunctionmap* _functionmap){
	if(!_functionmap)return;
	free_functionmapelement(_functionmap->_first);
	FREE_1(_functionmap,'F');
}// VALIDATED
#define FREE_FUNCTIONMAP(_functionmap,owner_functionmap) free_functionmap(disowned_functionmap(_functionmap,owner_functionmap))

// MDH@20JUL2019: might never get called, wel perhaps on internal functions when it goes out of scope???????
#ifndef __PRODUCTION__
/**
 * @brief returns \p _userfunction owned by \p owner_userfunction
 * 
 * @param _userfunction 
 * @param owner_userfunction 
 * @return Muserfunction* \p _userfunction owned by \p owner_userfunction
 */
Muserfunction* owned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction){
	if(!_userfunction)return NULL;
	owned_list(_userfunction->_bodyCommandList,Msubowner(owner_userfunction,1));
	// replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
	return OWNED(_userfunction,owner_userfunction);
}
/**
 * @brief returns \p _userfunction disowned by \p owner_userfunction
 * 
 * @param _userfunction 
 * @param owner_userfunction 
 * @return Muserfunction* \p _userfunction disowned by \p owner_userfunction
 */
Muserfunction* disowned_userfunction(Muserfunction * const _userfunction,Mallocationowner owner_userfunction){
	if(!_userfunction)return NULL;
	disowned_list(_userfunction->_bodyCommandList,owner_userfunction);
	// replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
	return DISOWNED(_userfunction,owner_userfunction);
}
#endif
/**
 * @brief frees \p _userfunction
 * 
 * @param _userfunction 
 */
void free_userfunction(Muserfunction* _userfunction){
	if(NULL==_userfunction)return;
	///////////if(_userfunction->_parameterMap)free_map(_userfunction->_parameterMap);
	// NOTE do NOT call free_value() on the body token value, instead NULL it so the reference count of the value is decremented!!!!
	free_list(_userfunction->_bodyCommandList);
	// replacing: assignValue(&_userfunction->_bodyTokenValue,NULL); // replacing: if(_userfunction->_bodyTokenValue)free_value(_userfunction->_bodyTokenValue);
	FREE_1(_userfunction,'U');
}/* VALIDATED */
// END RELEASERS

// Menvironment stuff
#ifndef __PRODUCTION__
/**
 * @brief returns \p _environment owned by \p owner_environment
 * 
 * @param _environment 
 * @param owner_environment 
 * @return Menvironment* \p _environment owned by \p owner_environment
 */
Menvironment* owned_environment(Menvironment* _environment,Mallocationowner owner_environment){
	if(NULL==_environment)return NULL;
	if(amVerboseDebugging())output("Taking over ownership environment.\n");
	owned_chars(_environment->_name,Msubowner(owner_environment,1));
	owned_map(_environment->_variableMap,Msubowner(owner_environment,1));
	if(_environment->_functionMap!=NULL)
		owned_functionmap(_environment->_functionMap,Msubowner(owner_environment,1)); // all functions also need to change their ownership
	/*
	if(_environment->blockCommandList!=NULL)
		owned_list(_environment->blockCommandList,Msubowner(owner_environment,1)); // MDH@18MAR2024
	*/
	return OWNED(_environment,owner_environment);
}
/**
 * @brief returns \p _environment disowned by \p owner_environment
 * 
 * @param _environment 
 * @param owner_environment 
 * @return Menvironment* \p _environment disowned by \p owner_environment
 */
Menvironment* disowned_environment(Menvironment* _environment,Mallocationowner owner_environment){
	if(NULL==_environment)return NULL;
	/// do not use output here!!! if(amVerboseDebugging())output("Releasing ownership environment '%s'.\n",_environment->_name->chars);
	disowned_chars(_environment->_name,owner_environment);
	/// do not use output here: if(amVerboseDebugging())output("\tEnvironment name ownership released.\n");
	disowned_map(_environment->_variableMap,owner_environment);
	/// do not use output here!!! if(amVerboseDebugging())output("\tEnvironment variable map ownership released.\n");
	if(_environment->_functionMap!=NULL){
		disowned_functionmap(_environment->_functionMap,owner_environment); // all functions also need to be disowned
		/// same here: if(amVerboseDebugging())output("\tEnvironment function map ownership released!");
	}
	/*
	if(_environment->blockCommandList!=NULL)
		disowned_list(_environment->blockCommandList,owner_environment); // MDH@18MAR2024: the block command list need to be disowned as well
		*/
	///////if(amVerboseDebugging())output("Environment function map ownership released.\n");
	// disowned_map(_environment->_functionMap);
	return DISOWNED(_environment,owner_environment);
}
#endif
void free_environment(Menvironment* _environment/*,Mallocationowner owner_environment*/){
	if(_environment!=NULL){
		if(_environment->_name!=NULL){
				// output("Freeing environment name '%s'.\n",_environment->_name->chars); // DEBUG
				freeChars(_environment->_name/*,owner_environment*/);
				_environment->_name=NULL;
		}
		// output("Releasing the parent.\n"); // DEBUG
		assignValue(&_environment->_parent,NULL); // MDH@03FEB2020 replacing:
		// output("Releasing the execution.\n"); // DEBUG
		assignValue(&_environment->execution,NULL); // MDH@03FEB2020 replacing: _environment->_execution=NULL;
		// output("Freeing the variable map.\n"); // DEBUG

		/* MDH@16APR2024: we do NOT want to delete the reference to the parent variable map which would be the first variable in the environment's variable map
		if(strcmp("`",_environment->_variableMap->_first->_variable->_name))
		*/
		free_map(_environment->_variableMap/*,owner_environment*/);
		// free_map(_environment->_functionMap); // MDH@04MAR2020: TODO do we need this??????
		/* MDH@10JUL2019: only Menvironment has a function map!!   
				MDH@20JUL2019: NO user functions may also contain a function map, which is referenced in a user function execution environment
											and indeed being a referenced they should not be freed (otherwise we would loose these nested functions on
											freeing the execution environment)
		if(_environment->_functionMap)free_functionmap(_environment->_functionMap);
		*/
		// MDH@18MAR2024: free any block command list TODO is such a list owned somehow????? if so we'd have to know who owns it calling FREE_LIST
		/*
		if(_environment->blockCommandList!=NULL)
			free_list(_environment->blockCommandList);*/
		FREE_1(_environment,'E'/*,owner_environment*/);
	}
}/* VALIDATED */
/**
 * @brief returns a new M environment
 * 
 * @return Menvironment* a new M environment
 */
Menvironment* __environment(){Mallocationowner owner=getOwner(__LINE__);
	Menvironment* _environment=CALLOC_1(sizeof(Menvironment),'E',owner);
	if(NULL==_environment){outputError("Failed to create an environment");return NULL;}
	_environment->_variableMap=CALLOC_1(sizeof(Mmap),'M',Msubowner(owner,1)); // ascertain that the environment contains a variable map
	if(NULL==_environment->_variableMap){FREE_ENVIRONMENT(_environment,owner);_environment=NULL;outputError("Failed to create the new environment variable map");}
	return disowned_environment(_environment,owner);
}/* VALIDATED */
// MDH@25FEB2021: on occasion it's useful to be able to initialize a new environment with a variable map somehow
//				NOTE _variableMap essentially needs to be a disowned map, otherwise we cannot take over membership
/**
 * @brief returns a new M environment containing M variable map \p _variableMap
 * 
 * @param _variableMap 
 * @return Menvironment* 
 */
Menvironment* _getEnvironment(Mmap* _variableMap){Mallocationowner owner=getOwner(__LINE__);
	Menvironment* _environment=CALLOC_1(sizeof(Menvironment),'E',owner);
	if(NULL==_environment){outputError("Failed to create an environment");return NULL;}
	_environment->_variableMap=(_variableMap?owned_map(_variableMap,Msubowner(owner,1)):CALLOC_1(sizeof(Mmap),'M',Msubowner(owner,1)));
	if(NULL==_environment->_variableMap){FREE_ENVIRONMENT(_environment,owner);_environment=NULL;outputError("Failed to create the new environment variable map");}
	return disowned_environment(_environment,owner);
}/* VALIDATED */

/**
 * @brief returns the immediate parent of M environment \p _environment
 * 
 * @param _environment 
 * @return Menvironment* the immediate parent of M environment \p _environment
 */
Menvironment* getEnvironmentParent(Menvironment const * const _environment){
	return(_environment&&_environment->_parent?getValueEnvironment(_environment->_parent):NULL);
}/* VALIDATED */
/**
 * @brief returns the new M string containing the full name of M environment \p _environment
 * 
 * @param _environment 
 * @return Mstring* the new M string containing the full name of M environment \p _environment
 */
Mstring* _getEnvironmentName(Menvironment const * _environment){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _environmentName=owned_string(__string(),owner);
	if(_environmentName!=NULL){
		Mstring* p=_environmentName;
		while(p!=NULL&&_environment!=NULL){
			if(string_length(p))p=string_insert_char(p,0,'.');
			if(_environment->_name!=NULL){
				///////output("Prepending '%s'.\n",_environment->_name->chars);
				p=string_prepend(p,_environment->_name->chars);
			}
			/////// printf("Prepending '%s'.\n",_environment->_name);
			_environment=getEnvironmentParent(_environment); // MDH@03MAR2020 replacing: _environment->_parent;
		}
		if(p!=NULL)return disowned_string(_environmentName,owner);
		FREE_STRING(_environmentName,owner);
	}
	return NULL;
}

// additional function for wrapping environments and functions
/**
 * @brief returns a new M value hosting M function \p _function
 * @param _function
 * @return a new M value hosting M function \p _function
 */
Mvalue* _getValueOfFunction(Mfunction* _function/*,Mallocationowner owner_function*/){
	if(NULL==_function)return NULL;
	bool disowned_function=Misdisowned(_function);
	if(amVerbose())
		output("Wrapping a %s function.\n",(disowned_function?"disowned":"owned"));
	Mvalue* _value=__value("function");
	if(NULL==_value){
		if(disowned_function)free_function(_function);
		return NULL;
	}
	if(amVerbose())
		outputInfo("Binding the function to the value");
	_value->type=VT_FUNCTION;
	_value->value._function=(disowned_function?owned_function(_function,owner_value_data):_function);
	if(amVerbose())
		output("%s function wrapped.\n",(disowned_function?"Disowned":"Owned"));
	return _value;
}/* VALIDATED */

/**
 * @brief returns a new M value hosting M environment \p _environment
 * @param _environment
 * @return a new M value hosting M environment \p _environment
 */
Mvalue* _getValueOfEnvironment(Menvironment* _environment/*,Mallocationowner owner_environment*/){
	Mvalue* _valueOfEnvironment=NULL;
	if(_environment!=NULL){
		bool disowned_environment=Misdisowned(_environment);
		_valueOfEnvironment=__value("environment");
		if(_valueOfEnvironment!=NULL){
			_valueOfEnvironment->type=VT_ENVIRONMENT;
			_valueOfEnvironment->value._environment=(disowned_environment?owned_environment(_environment,owner_value_data):_environment);
		}else
		if(disowned_environment)free_environment(_environment);
	}
	return _valueOfEnvironment;
}/* VALIDATED */

// Mfile support
Mmap* _getFileStatPropertyMap(Mfile const * const _file){Mallocationowner owner=getOwner(__LINE__);
	if(_file!=NULL&&_file->_name!=NULL){
		Mmap* _fileStatMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
		if(_fileStatMap!=NULL){
			// MDH@11MAY2024: force updating the stats if _file->staterrno does not equal 0, if it equals to 0 we're assuming it's up to date, but trying again even if staterrno equals errno (when positive) just in case it changed!!!
			if(_file->staterrno!=0)fUpdateStats(_file,false); // do NOT report here, because you'd get a circular reference that way!!
			if(_file->staterrno>0){
				appendedToMap(_fileStatMap,owner,"errno",_getValueOfInteger(_getInteger(_file->staterrno))); // NOTE integer returned by _getInteger freed by _getValueOfInteger when failing to wrap it
				appendedToMap(_fileStatMap,owner,"error",_getValueOfText(_getSingleQuotedText(strerror(_file->staterrno))));
			}
			if(_file->staterrno==0){ // the stats have been determined!!!
				appendedToMap(_fileStatMap,owner,"exists",_getTextValue("'yes"));
				struct stat filestat=_file->stat; // MDH@02MAY2024
				// File permissions
				Mstring* _permissionsText=owned_string(_getFilePermissionsText(filestat.st_mode),owner);
				if(_permissionsText!=NULL)
					if(appendedToMap(_fileStatMap,owner,"permissions",_getTextValue(string(_permissionsText)))>0)
						///output("Permissions property added to the file stat map.\n")
						;
				/* replacing:
				if(_permissions!=NULL){
					// File access property can tell us more than whether it is a directory
					if(S_ISDIR(filestat.st_mode))string_append_char(_permissions,'d');else
					if(S_ISREG(filestat.st_mode))string_append_char(_permissions,'f');else string_append_char(_permissions,'?');
					 // TODO we might need to consider other types as well like links, devices and the like
					if(filestat.st_mode & R_OK)string_append_char(_permissions,'r');
					if(filestat.st_mode & W_OK)string_append_char(_permissions,'w');
					if(filestat.st_mode & X_OK)string_append_char(_permissions,'x');
					if(appendedToMap(_fileStatMap,owner,"permissions",_getTextValue(string(_permissions)))>0)
						///output("Permissions property added to the file stat map.\n")
						;
					FREE_STRING(_permissions,owner);
				}else
					outputError("Failed to represent the file permissions");
				*/
				// File size property
				Minteger* _integer=owned_integer(_getInteger(filestat.st_size),owner);
				if(_integer!=NULL){
					if(appendedToMap(_fileStatMap,owner,"size",_getValueOfInteger(disowned_integer(_integer,owner)))>0)
						///output("File size added to file stat map.\n")
					;
				}else
					outputError("Failed to represent the file size");

				// Get file creation time in seconds and convert seconds to date and time format
				struct tm dt = *(gmtime(&filestat.st_ctime));
				Mstring* _created=owned_string(_getString("'"),owner);
				if(_created!=NULL){
					if(string_setlength(_created,51))string_setlength(_created,1+strftime(_created->_chars->chars+1,50,"%Y-%m-%d %H:%M:%S",&dt));
					if(appendedToMap(_fileStatMap,owner,"created",_getTextValue(string(_created)))>0)
						///output("File creation timestamp added to the file stat map.\n")
						;
					FREE_STRING(_created,owner);
				}
				// from: printf("\nCreated on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900,dt.tm_hour, dt.tm_min, dt.tm_sec);

				// File modification time
				dt = *(gmtime(&filestat.st_mtime));
				Mstring* _modified=owned_string(_getString("'"),owner);
				if(_modified!=NULL){
					if(string_setlength(_modified,51))string_setlength(_modified,1+strftime(_modified->_chars->chars+1,50,"%Y-%m-%d %H:%M:%S",&dt));
					if(appendedToMap(_fileStatMap,owner,"modified",_getTextValue(string(_modified)))>0)
						///output("File modification timestamp added to the file stat map.\n")
						;
					FREE_STRING(_modified,owner);
				}
			}else
				appendedToMap(_fileStatMap,owner,"exists",_getTextValue("'no"));

			// from: printf("\nModified on: %d-%d-%d %d:%d:%d", dt.tm_mday, dt.tm_mon, dt.tm_year + 1900, dt.tm_hour, dt.tm_min, dt.tm_sec);
			return disowned_map(_fileStatMap,owner);
		}
	}
	return NULL;
}

static bool fCanRead(char const * const filename){
	return(access(filename,R_OK)==0);
}
static bool fCanExecute(char const * const filename){
	return(access(filename,X_OK)==0);
}
static bool fCanWrite(char const * const filename){
	return(access(filename,W_OK)==0);
}
Mmap* _getFileAccessPropertyMap(Mfile const * const _file){Mallocationowner owner=getOwner(__LINE__);
	if(_file!=NULL&&_file->_name!=NULL){
		Mmap* _fileAccessMap=owned_map(_getMapOfType(VT_UNDEFINED),owner);
		////output("Adding file access properties.\n");
		bool fileExists=(fExists(_file,false)==M_TRUE); // NOTE: NOT using fExists() here, because we only want to use the access() function
		if(fileExists){
			appendedToMap(_fileAccessMap,owner,"exists",_getTextValue("'yes"));
			bool aRegularFile=fIsRegularFile(_file,false);
			appendedToMap(_fileAccessMap,owner,"regularfile",_getTextValue(aRegularFile?"'yes":"'no"));
			if(aRegularFile){
				appendedToMap(_fileAccessMap,owner,"executable",_getTextValue((fCanExecute(string(_file->_name))?"'yes":"'no")));
				appendedToMap(_fileAccessMap,owner,"readable",_getTextValue((fCanRead(string(_file->_name))?"'yes":"'no")));
				appendedToMap(_fileAccessMap,owner,"writable",_getTextValue((fCanWrite(string(_file->_name))?"'yes":"'no")));
			}
		}else
			appendedToMap(_fileAccessMap,owner,"exists",_getTextValue("'no"));
		return disowned_map(_fileAccessMap,owner);
	}
	return NULL;
}

/**
 * @brief returns a new M map containing the properties of M file \p _file
 * 
 * @param _file 
 * @return Mmap* a new M map containing the properties of M file \p _file
 */
Mmap* _getFilePropertyMap(Mfile const * const _file){Mallocationowner owner=getOwner(__LINE__);
	if(_file!=NULL){
		Mmap* _map=owned_map(_getMapOfType(VT_UNDEFINED),owner);
		if(_map!=NULL){
			
			if(_file->_name!=NULL){
				///output("Adding the name of the file to the file property map.\n");
				Mstring* _filename=owned_string(_getString("'"),owner);
				if(_filename!=NULL){
					if(string_append(_filename,string(_file->_name))!=NULL)
						if(appendedToMap(_map,owner,"name",_getTextValue(string(_filename)))>0)
							///output("File name added to file property map.\n")
							;
					FREE_STRING(_filename,owner);
				}
			}

			if(_file->_f!=NULL){ // the file is currently open
				///output("Adding the properties of the opened file.\n");
				appendedToMap(_map,owner,"status",_getValueOfText(_getText("'opened")));
				// show the mode the file was opened in
				if(_file->_mode!=NULL){
					Mstring* _filemode=owned_string(_getString("'"),owner);
					if(_filemode!=NULL){
						if(string_append(_filemode,_file->_mode)!=NULL)
							if(appendedToMap(_map,owner,"mode",_getTextValue(string(_filemode)))>0)
								///output("File mode appended to file property map.\n")
								;
						FREE_STRING(_filemode,owner);
					}
				}else
					outputBug("File mode of opened file vanished!");
				/* replacing:
				Mstring* _openmode=owned_string(_getString("'"),owner);
				if(_openmode!=NULL){
					if(_file->mode[0])string_append_char(_openmode,_file->mode[0]);
					if(_file->mode[1])string_append_char(_openmode,_file->mode[1]);
					if(_file->mode[2])string_append_char(_openmode,_file->mode[2]);
					appendedToMap(_map,owner,"mode",_getTextValue(string(_openmode)));
					FREE_STRING(_openmode,owner);
				}
				*/
				///output("Adding the position in the file.\n");
				fpos_t filepos=fPosition(_file);
				Minteger* _integer=owned_integer(_getInteger(filepos),owner);
				appendedToMap(_map,owner,"position",_getValueOfInteger(disowned_integer(_integer,owner)));
			}else{
				///output("Unopened file.\n");
				appendedToMap(_map,owner,"status",_getValueOfText(_getText("'unopened")));
			}

			//                the information we get through calling (f)stat is placed inside the "stat" property
			/////output("Adding file stat properties.\n");
			Mmap* _fileStatMap=owned_map(_getFileStatPropertyMap(_file),owner);
			if(_fileStatMap!=NULL)appendedToMap(_map,owner,"stat",_getValueOfMap(disowned_map(_fileStatMap,owner)));

			// can we use access to get information on what access we can have to the 'file'???
			Mmap* _fileAccessMap=owned_map(_getFileAccessPropertyMap(_file),owner);
			if(_fileAccessMap!=NULL)appendedToMap(_map,owner,"access",_getValueOfMap(disowned_map(_fileAccessMap,owner)));

			return disowned_map(_map,owner);

		}else
			outputError("Failed to create the file property map");
	}
	return NULL;

}

/**
 * @brief returns M_TRUE when \p file exists, M_FALSE when it does not, or M_LL_INVALID when \p file it is not a (named) file
 * 
 * @param file 
 * @return long long M_TRUE when \p file exists, M_FALSE when it does not, or M_LL_INVALID when \p file it is not a (named) file
 */
long long fExists(Mfile * const file,bool report){
	if(file!=NULL&&file->_name!=NULL){
		// ok, let's attempt to use file stats to determine existence as our first method
		fUpdateStats(file,report);
		if(file->staterrno==0)return M_TRUE; // if we have file statistics it's reasonable to assume that the file exists
		if(access(file->_name,F_OK)==0)return M_TRUE; // the file or directory exists
		// ASSERT the file or directory does not exist
		if(report){
			switch(errno){
				case EACCES:output("%s'The permissions specified by amode (%d) are denied, or search permission is denied on a component of the path prefix",M_ERROR_PREFIX,F_OK);break;
				case EINTR:output("%s'Interrupted by a signal",M_ERROR_PREFIX);break;
				case EINVAL:output("%s'Invalid amode (%d)",M_BUG_PREFIX,F_OK);break;
				case ELOOP:output("%s'Too many levels of symbolic links or prefixes",M_ERROR_PREFIX);break;
				case ENAMETOOLONG:output("%s'The length of the file/directory name exceeds %d, or one of the parts of the file/directory name is longer than %d",M_ERROR_PREFIX,PATH_MAX,NAME_MAX);break;
				case ENOENT:output("%s'A component of the path isn't valid",M_ERROR_PREFIX);break;
				case ENOSYS:output("%s'The access() function isn't implemented for the filesystem underlying the file/directory specified");break;
				case ENOTDIR:output("%s'A component of the path isn't a directory",M_ERROR_PREFIX);break;
				case EROFS:output("%s'Write access was requested for a file residing on a read-only file system",M_ERROR_PREFIX);break;
			}
			output("' checking for the existence of file/directory '%s'.\n",string(file->_name));
		}
		return M_FALSE;
	}
	return M_LL_INVALID;
}
/**
 * @brief returns M_TRUE when M file \p file denotes an existing directory, M_FALSE otherwise
 * @details returns M_LL_INVALID when \p file equals NULL or does not have a name
 * @param file 
 * @param report 
 * @return long long M_TRUE when \p file denotes an existing directory, M_FALSE or M_LL_INVALID (see @details) otherwise 
 */
long long fIsDir(Mfile * const file,bool report){
	long long result=fExists(file,report); 
	// file needs to exist as prerequisite to testing whether it is a directory
	if(result==M_TRUE){ // an existing file/directory
		if(file->staterrno!=0){
			fUpdateStats(file,report); // a chance to become zero!!!
			if(file->staterrno!=0){
				output("%sExisting file '%s' failed to update the file stats of file/directory '%s'.\n");
				result=M_FALSE;
			}
		}
		if(result==M_TRUE&&!S_ISDIR(file->stat.st_mode))result=M_FALSE;
	}
	return result;
}
/**
 * @brief returns M_TRUE when M file \p file denotes an existing file, M_FALSE otherwise
 * @details returns M_LL_INVALID when \p file equals NULL or does not have a name
 * @param file 
 * @param report 
 * @return long long M_TRUE when \p file denotes an existing file, M_FALSE or M_LL_INVALID (see @details) otherwise 
 */
long long fIsRegularFile(Mfile * const file,bool report){
	long long result=fExists(file,report); 
	// file needs to exist as prerequisite to testing whether it is a directory
	if(result==M_TRUE){ // an existing file/directory
		if(file->staterrno!=0){
			fUpdateStats(file,report); // a chance to become zero!!!
			if(file->staterrno!=0){
				output("%sExisting file '%s' failed to update the file stats of file/directory '%s'.\n");
				result=M_FALSE;
			}
		}
		if(result==M_TRUE&&!S_ISREG(file->stat.st_mode))result=M_FALSE;
	}
	return result;
}

/**
 * @brief returns a new M file with name \p filename 
 * @details return NULL on failure
 * @param filename 
 * @return Mfile* a new M file with name \p filename
 */
Mfile* _getFile(char const * const filename){Mallocationowner owner=getOwner(__LINE__);
	Mfile* _file=owned_file(__file(),owner); // get an owned new file instance
	// the file might not exist in which case we could get rid of _file->_stat???
	if(_file!=NULL){
		if(filename!=NULL&&strlen(filename)){
			_file->_name=owned_string(_getString(filename),Msubowner(owner,1)); // bind _filename to _file->_name (ownership one level down)
			if(NULL==_file->_name){
				output("%sFailed to set the name of the file to '%s'.\n",M_ERROR_PREFIX,filename);
				FREE_FILE(_file,owner);
				return NULL;
			}
			fUpdateStats(_file,false); // MDH@02MAY2024: initialization of the stats delegated to fUpdateStats()
			return disowned_file(_file,owner);
		}
		/*
		// MDH@01MAY2024: if the file exists we want statistics!!!
		if(NULL==_file->_stat)_file->_stat=CALLOC_1(sizeof(struct stat),'f',Msubowner(owner,1)); // no need to disown here!!!!
		if(_file->_stat!=NULL){
			if(stat(string(_file->_name),_file->_stat)!=0){FREE_DISOWNED_1(_file->_stat,'f',owner);_file->_stat=NULL;}
		}else
			outputError("Unable to obtain the file stats");
		*/
	}
	return NULL;
}
/**
 * @brief returns the M file wrapped in M value \p filenameValue
 * 
 * @param filenameValue 
 * @return Mfile* the M file wrapped in M value \p filenameValue
 */
static Mfile* _getFileWithName(Mvalue const * const filenameValue){
	if(filenameValue!=NULL){
		/* don't actually want this!!!
		if(filenameValue->type==VT_FILE)
			return filenameValue->value._file;
		*/
		if(filenameValue->type==VT_TEXT){ // supposedly always a copy of the input argument so we can bind it to _file in _file->_name
			if(filenameValue->value._text!=NULL){
				char* filename=filenameValue->value._text->_c;
				Mfile* _file=_getFile(filename);
				if(_file!=NULL)return _file;
				output("%sFailed to create a file object with name '%s'.\n",M_ERROR_PREFIX,filename);
			}else
				outputBug("Filename vanished");
		}else
			outputError("Assumed filename of wrong type");
	}else
		outputError("No filename specified");
	return NULL;
}
/**
 * @brief returns the new M value wrapping M file \p _file
 * 
 * @param _file 
 * @return Mvalue* the new M value wrapping M file \p _file
 */
Mvalue* _getValueOfFile(Mfile const * const _file){
	if(NULL==_file)return NULL;
	bool disowned_file=Misdisowned(_file);
	////output(disowned_file?"Disowned file":"Owned file");
	Mvalue* _value=__value("file");
	if(NULL==_value){
		if(disowned_file)free_file(_file);
		return NULL;
	}
	_value->type=VT_FILE;
	// MDH@12JUN2020: TODO supposedly this is a bit of a problem actually taking over the ownership of an environment completely
	_value->value._file=(disowned_file?owned_file(_file,owner_value_data):_file);
	return _value;
}

/**
 * @brief returns M_TRUE if M file \p _file is an existing and readable file, M_FALSE or M_LL_INVALID otherwise
 * @details returns M_LL_INVALID when \p _file does not denote an existing file
 * @param _file 
 * @return M_TRUE if M file \p _file is an existing and readable file, M_FALSE or M_LL_INVALID otherwise
 */
long long fIsReadable(Mfile const * const _file,bool report){
	long long result=M_LL_INVALID;
	if(_file!=NULL){
		if(_file->_f!=NULL){ // an open file
			// we may inspect the mode in which the file was opened to determine if it is readable or not
			int l=strlen(_file->_mode);
			result=((_file->_mode[0]!='w')||(l>1&&_file->_mode[1]=='+')||(l>2&&_file->_mode[2]=='+')?M_TRUE:M_FALSE);
		}else{ // not an opened file!!
			// anything that is not an existing regular file is not considered readable
			result=fIsRegularFile(_file,report); // NOTE will call fExists() as well
			if(result==M_TRUE&&!access(_file->_name,R_OK))result=M_FALSE;
			/* replacing:
			// an unopened file is readable when it exists, is not a directory and has the 'r' access flag set
			// I suppose an open file is also readable when it has not been opened in write-only mode
			if(_file->staterrno<0)fUpdateStats(_file,false);
			if(_file->staterrno==0&&!S_ISDIR(_file->stat.st_mode)&&_file->stat.st_mode&R_OK)
			return true;
			*/
		}
	}else
	if(report)
		outputError("No file specified to determine the readability of");
	return result;
}
/**
 * @brief returns M_TRUE if \p _file is a writable file, M_FALSE or M_LL_INVALID otherwise
 * @details returns M_LL_INVALID if \p _file does not denote an existing file
 * @param _file 
 * @return M_TRUE if \p _file is a writable file, M_FALSE or M_LL_INVALID otherwise
 */
long long fIsWriteable(Mfile const * const _file,bool report){
	long long result=M_LL_INVALID;
	if(_file!=NULL){
		if(_file->_f!=NULL){ // an open file
			// we may inspect the mode in which the file was opened to determine if it is readable or not
			int l=strlen(_file->_mode);
			result=((_file->_mode[0]!='r')||(l>1&&_file->_mode[1]=='+')||(l>2&&_file->_mode[2]=='+')?M_TRUE:M_FALSE);
		}else{ // not an opened file!!
			// anything that is not an existing regular file is not considered readable
			result=fIsRegularFile(_file,report); // NOTE will call fExists() as well
			if(result==M_TRUE&&!access(_file->_name,W_OK))result=M_FALSE;
			/* replacing:
			// an unopened file is readable when it exists, is not a directory and has the 'r' access flag set
			// I suppose an open file is also readable when it has not been opened in write-only mode
			if(_file->staterrno<0)fUpdateStats(_file,false);
			if(_file->staterrno==0&&!S_ISDIR(_file->stat.st_mode)&&_file->stat.st_mode&R_OK)
			return true;
			*/
		}
	}else
	if(report)
		outputError("No file specified to determine the readability of");
	return result;
	/* replacing:
	if(NULL==_file)return false;
	if(_file->_f!=NULL){
		int l=strlen(_file->_mode);
		return((_file->_mode[0]!='r')||(l>1&&_file->_mode[1]=='+')||(l>2&&_file->_mode[2]=='+'));
	}else{
		if(_file->staterrno<0)fUpdateStats(_file,false);
		if(_file->staterrno==0&&!S_ISDIR(_file->stat.st_mode)&&_file->stat.st_mode&W_OK)
			return true;
	}
	return false;
	*/
	// a file is writeable when it exists, is not open yet, is not a directory and has the 'w' access flag set
}
static long long fIsOpenedInBinaryMode(Mfile const * const file){
	return(file!=NULL&&file->_f!=NULL&&file->_mode!=NULL?(strchr(file->_mode,'b')!=NULL?M_TRUE:M_FALSE):M_LL_INVALID);
}

// MDH@30APR2024: most of the M functions delegate to these internal functions that are easier to use directly, so we won't have to Mvalue wrap a lot of data
/**
 * @brief returns M_TRUE when \p file is deleted, M_FALSE otherwise
 * @details updates the statistics of \p file accordingly, if possible
 * @param file the (unopened) M file object
 * @return M_TRUE on success
 * @return M_FALSE on failure
 */
long long fDeleted(Mfile * const _file,Mallocationowner owner_file){
	// can only delete an existing file that is not currently open
	bool result=M_LL_INVALID;
	if(_file!=NULL&&_file->_name!=NULL){
		if(NULL==_file->_f){ // a file with a name that is not currently open
			result=fIsRegularFile(_file,true);
			if(result==M_TRUE){
				if(remove(string(_file->_name))){ // failure because remove() returns 0 on success
					result=M_FALSE;
					output("%sFailed to delete file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
				}
			}else
				output("%sCan't delete '%s': it is not a regular (existing) file.\n",M_ERROR_PREFIX,string(_file->_name));
			/* replacing:
			// if remove returns a non-zero value, removing the file failed!!!!
			if(file->staterrno<0)fUpdateStats(file,true);
			if(file->staterrno==0){
				result=(remove(string(file->_name))?M_FALSE:M_TRUE);
				if(result==M_TRUE){
					// we may assume the file no longer exists, so we should fail getting the stats
					file->staterrno=INT_MIN; // indicating that the stats are now dirty
					////* replacing:
					///if(stat(string(file->_name),file->_stat)!=0){
					///	FREE_DISOWNED_1(file->_stat,'f',owner_file);
					///	file->_stat=NULL;
					///}else
					///	outputError("Failed to remove the stats of the deleted file");
					///
				}else // failure, so the stats stay!!!
					output("%sFailed to delete file '%s'.",M_ERROR_PREFIX,string(file->_name));
			}else{
				result=M_TRUE; // safe to return true as well!!!
				output("%sFile '%s' cannot be deleted: it does not exist!\n",M_ERROR_PREFIX,string(file->_name));
			}
			*/
		}else
			output("%sIt is not allowed to delete an opened file '%s'!\n",M_ERROR_PREFIX,string(_file->_name));
	}else
	if(_file!=NULL)
		outputError("Cannot delete an unnamed file");
	else
		outputError("No file to delete specified");
	return result;
}
/**
 * @brief returns M_TRUE on successful opening \p file, M_FALSE otherwise
 * @details returns M_LL_INVALID if \p file does not denote a file
 * @param file the M file to open
 * @return M_TRUE on success, M_FALSE otherwise
 */
long long fOpened(Mfile * const file,Mallocationowner owner_file,char const * openmodeSpec,bool allowExistingWrite,bool report){
	// ASSERT openmodeSpec (as checked in Mfopen) should be a valid mode specification
	long long result=M_LL_INVALID;
	if(file!=NULL){
		/* MDH@03MAY2024: what I want to do here is attempt to open the file
		                  and if we succeed store openmodeSpec in file->_mode
		if(file->staterrno<0)fUpdateStats(file,false);
		////////outputChar('X');
		char openmode=(openmodeSpec!=NULL&&*openmodeSpec?*openmodeSpec:(file->staterrno==0?'r':'w')); // if mode_value is defined, it must be of type VT_TEXT, and the first character should be 'a', 'r' or 'w' to be a valid mode
		if(openmode>=65&&openmode<=90)openmode+=32; // ascertain to have a lowercase mode specification
		char mode[4]={openmode}; // initialize mode to openmode NOTE mode always needs to end with '\0'
		if(openmodeSpec!=NULL&&*openmodeSpec){ // accepting two additional mode characters '+' and 'b'
			// let's simply copy the characters (even if they are wrong)
			mode[1]=*(++openmodeSpec);if(mode[1]>=65&&mode[1]<=90)mode[1]+=32;
			if(mode[1])mode[2]=*(++openmodeSpec);
		}else // for optimal flexibility allow for writing as well
			mode[1]='+';
		*/
		// how about checking whether the mode_value argument is correct first??????
		// let's NOT allow overwriting an existing file!!!
		/* there's a better way to do this
		if(file->staterrno<0)fUpdateStats(file,report);
		// if we leave file->_mode the way it was, we allow the user to reopen the file in the same mode
		char openmode=(openmodeSpec!=NULL?*openmodeSpec:'\0');
		if(!openmode&&file->_mode!=NULL)openmode=*file->_mode;
		if(!openmode){
			openmode=(file->staterrno==0?'r':'w')); // if mode_value is defined, it must be of type VT_TEXT, and the first character should be 'a', 'r' or 'w' to be a valid mode
		}
		char mode[4]={openmode}; // initialize mode to openmode NOTE mode always needs to end with '\0'
		if(openmodeSpec!=NULL&&*openmodeSpec){ // accepting two additional mode characters '+' and 'b'
			// let's simply copy the characters (even if they are wrong)
			mode[1]=*(++openmodeSpec);if(mode[1]>=65&&mode[1]<=90)mode[1]+=32;
			if(mode[1])mode[2]=*(++openmodeSpec);
		}else // for optimal flexibility allow for writing as well
			mode[1]='+';
		/////output("Requested open mode: '%s'.\n",mode);
		*/
		/////// if we use fExists() we do NOT need to update the file stats... if(file->staterrno!=0)fUpdateStats(file,report);
		// let's determine the opening mode
		bool fileExists=(fExists(file,true)==M_TRUE);
		char* mode=openmodeSpec;
		if(NULL==mode||!*mode)mode=file->_mode;
		if(NULL==mode||!*mode){
			mode=(fileExists?"r+":"w");
			output("Will attempt to open file '%s' in system default mode '%s'.\n",string(file->_name),mode);
		}else
		if(NULL==openmodeSpec)
			output("Using the last used file mode '%s' to open file '%s' in.\n",file->_mode,string(file->_name));
		result=(file->_f!=NULL?M_TRUE:M_FALSE);
		if(result==M_FALSE){ // not currently opened
			if(report)
				output("Opening the %sexisting file '%s' in mode '%s'.\n",(!fileExists?"non-":""),string(file->_name),mode);
			if(*mode=='w'||*mode=='r'||*mode=='a'){ // a valid open mode
				if(*mode!='w'||file->staterrno!=0||allowExistingWrite){ // but do NOT allow deleting existing content unless allowExistingWrite flag is set!!!
					// let's initialize the default mode
					// try to open the file
					openFile(file,owner_file,mode,report); // if I'm doing the reporting openFile shouldn't
					// get rid of the current file mode whatever it is
					if(file->_mode!=NULL){FREE(file->_mode,1+strlen(file->_mode),-'"');file->_mode=NULL;output("Registered file mode released.\n");}
					if(file->_f!=NULL){ // successfully opened the file in mode 'mode'
						file->_mode=_strdup(mode); // register the actual mode the file was opened in
						if(file->_mode!=NULL)result=M_TRUE;else output("%sFailed to register '%s' as the mode file '%s' was opened in.\n",M_ERROR_PREFIX,mode,string(file->_name));
					}else
						output("%sFailed to open file '%s' in mode '%s'.\n",M_ERROR_PREFIX,string(file->_name),mode);
					/* replacing and augmenting:
					_file->_f=fopen(string(_file->_name),mode);
					if(_file->_f){_file->mode[0]=mode[0];_file->mode[1]=mode[1];_file->mode[2]=mode[2];} // remember the opening mode when the file was successfully opened
					*/
				}else
				if(*mode=='w')
					output("%sCan't overwrite existing content in '%s'.\n",M_ERROR_PREFIX,string(file->_name));
				else
					output("%Can't open directory '%s'.\n",M_ERROR_PREFIX,string(file->_name));
			}else
				output("%sCan't open file '%s': invalid mode '%s'.\n",M_ERROR_PREFIX,string(file->_name),mode);
		}else
		if(strcmp(mode,file->_mode))
			output("%sFile '%s' already open in mode '%s' instead of the requested mode '%s'!\n",M_WARNING_PREFIX,string(file->_name),file->_mode,mode);
		else
			output("%sFile '%s' already open!\n",M_WARNING_PREFIX,string(file->_name));
	}
	return result;
}
/**
 * @brief closes \p file
 * 
 * @param file the file to close
 * @return long long M_TRUE on success, M_FALSE on failure, or M_LL_INVALID when \p file equals NULL
 */
long long fClosed(Mfile * const file,Mallocationowner owner_file,bool report){
	if(file!=NULL&&file->_name!=NULL){
		if(file->_f!=NULL){
			///delegated now to closeFile!!! if(report)output("Closing file '%s'.\n",string(file->_name));
			if(!closeFile(file,report))return M_FALSE;
		}else
			output("%sFile '%s' already closed!",M_WARNING_PREFIX,string(file->_name));
		return M_TRUE;
	}
	return M_LL_INVALID;
	/* replacing:
	long long result=(file!=NULL?(closeFile(file)?M_TRUE:M_FALSE):M_LL_INVALID);
	// closeFile() already takes care of clearing file->_f!!!! if(result==M_TRUE){FREE_DISOWNED_1(file->_f,'f',owner);file->_f=NULL;}
	return result;
	*/
}

/**
 * @brief reads (at most) \p numberOfBytes from file \p file
 * 
 * @param file the file to read from 
 * @param numberOfBytes the maximum number of bytes to read
 * @return Mstring* the bytes read
 */
Mstring* fRead(Mfile const * const file/*,Mallocationowner owner_file*/,long long numberOfBytes){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _bytesRead=NULL;
	if(file!=NULL){
		// before checking the mode to see if the file can be read from, we might need to open it
		// it's easiest to check whether it exists to start with, because if it doesn't it can't be read from anyway
		if(file->_f!=NULL){ // and opened
			if(fIsReadable(file,false)){ // and readable
				if(!feof(file->_f)){
					bool openedInBinaryMode=(fIsOpenedInBinaryMode(file)==M_TRUE);
					// if the file wasn't opened before, try to open it for reading 'text' (i.e. not binary)
					////////////if(NULL==file->_f)openFile(file,owner_file,"r+");
					// if the file can be read from, we do
					///if(file->_mode[1]=='+'||file->_mode[0]=='a'||file->_mode[0]=='r'){ // the mode is defined (i.e. unequal to it's initial value '\0')
					// let's read in blocks of 1024 bytes
					size_t numberOfBytesToReadAtOnce=(numberOfBytes<0?1024:numberOfBytes);
					if(openedInBinaryMode)
						_bytesRead=owned_string((numberOfBytes>0?_getStringOfLength(numberOfBytes):__string()),owner);
					else
						_bytesRead=owned_string(_getString("'"),owner); // start the string with a single quote for create a Mtext from it
					if(_bytesRead!=NULL){
						if(numberOfBytesToReadAtOnce>0){ // there are bytes requested to read
							// keep reading until we're done reading all requested bytes
							size_t l;
							while(numberOfBytes!=0){
								l=string_length(_bytesRead);
								// we have to ascertain to have sufficient room to store what we read
								output("Changing the string length from %llu to %llu.\n",l,l+numberOfBytesToReadAtOnce);
								if(NULL==string_setlength(_bytesRead,l+numberOfBytesToReadAtOnce)){
									output("%sFailed to allocate sufficient memory to read the bytes from file '%s' to.\n",M_ERROR_PREFIX,string(file->_name));
									break;
								}
								output("Length set to %llu.\n",_bytesRead->length);
								output("Reading %lld bytes from file '%s'.\n",numberOfBytesToReadAtOnce,string(file->_name));
								size_t numberOfBytesRead=fread(_bytesRead->_chars->chars+l,sizeof(unsigned char),numberOfBytesToReadAtOnce,file->_f);
								string_setlength(_bytesRead,l+numberOfBytesRead);
								if(numberOfBytesRead==0)break;
								if(feof(file->_f))break;
								if(numberOfBytes>0)numberOfBytes-=numberOfBytesRead;
							}
							/* replacing:
							// the file could be empty to start with
							while(numberOfBytes>0&&!feof(file->_f)){
								if(NULL==string_append_char(_bytesRead,fgetc(file->_f)))break;
								numberOfBytes--;
							}
							*/
							if(numberOfBytes>0)
								output("%sFailed to read %d %s from file '%s'.\n",M_ERROR_PREFIX,numberOfBytes,(openedInBinaryMode?"bytes":"characters"),string(file->_name));
							// succeeded when all bytes were read or we bumped into end-of-file!!
							if(numberOfBytes<=0||feof(file->_f))return disowned_string(_bytesRead,owner);
							FREE_STRING(_bytesRead,owner);
							}
					}else
						output("%sFailed to prepare for reading %s from file '%s'.\n",M_ERROR_PREFIX,(openedInBinaryMode?"bytes":"characters"),string(file->_name));
				}else
					output("%sCannot read from '%s': end-of-file reached.\n",M_ERROR_PREFIX,string(file->_name));
				////}
			}else
				output("%sCan't read from file '%s': it is not readable.\n",M_ERROR_PREFIX,string(file->_name));
		}else
			output("%sCan't read from file '%s': it is not open.\n",M_ERROR_PREFIX,string(file->_name));
	}else
		outputError("No file specified to read from");
	return NULL;
}

/**
 * @brief reports \p errorCode as returned by string_freadline() reading from file \p file
 * 
 * @param file the file read from
 * @param errorCode the error code returned by string_freadline() when reading from \p file
 */
static void reportFileReadErrorCode(Mfile const * const file,long long errorCode){
	if(errorCode==M_LL_INVALID)
		outputBug("Invalid arguments to the string_freadline() function call");
	else
	if(errorCode==1)
		output("%sOut of memory trying to read a text line from file '%s'.\n",M_ERROR_PREFIX,string(file->_name));
	else
	if(errorCode==2){
		int fileErrorCode=ferror(file->_f);
		if(fileErrorCode)
			output("%sError with error code %d trying to read a text line from file '%s'.\n",M_ERROR_PREFIX,fileErrorCode,string(file->_name));
		else
			output("%sUnknown error trying to read a text line from file '%s'.\n",M_ERROR_PREFIX,string(file->_name));
	}
}
/**
 * @brief reads a single text line from \p file
 * 
 * @param file the file to read the text line from
 * @return Mstring* the single text line (without EOLN)
 */
Mstring* fReadLine(Mfile const * const file/*,Mallocationowner owner_file*/){Mallocationowner owner=getOwner(__LINE__);
	Mstring* _bytesRead=NULL;
	if(file!=NULL){
		if(file->_f!=NULL){
			if(fIsReadable(file,false)){
				bool openedInBinaryMode=(fIsOpenedInBinaryMode(file)==M_TRUE);
				// if the file is not binary and can be read from
				////if(!openedInBinaryMode){ // the mode is defined (i.e. unequal to it's initial value '\0')
					if(!feof(file->_f)){ // MDH@27DEC2020 appended: to ascertain that NULL is returned
						_bytesRead=owned_string((openedInBinaryMode?__string():_getString("'")),owner); // NO do NOT start the string with a single quote for create a Mtext from it
						if(_bytesRead!=NULL){
							// string_freadline returns the number of characters (well bytes actually) read but not used (so those are the characters in the next line)
							unsigned char* linefeedCharacterPosition=NULL;
							long long errorCode=string_freadline(_bytesRead,file->_f,&linefeedCharacterPosition);
							if(errorCode!=0)reportFileReadErrorCode(file,errorCode);
							// no matter what errorCode is if we found a line we have to process it (and return it)
							// if end-of-line was not found but we did bump into end-of-file whatever was read needs to be returned
							if(NULL==linefeedCharacterPosition&&feof(file->_f))linefeedCharacterPosition=_bytesRead->_chars->chars+_bytesRead->length;
							long long readButNotInLine;
							if(linefeedCharacterPosition!=NULL){
								// MDH@03JUN2024: when opened in binary mode, store the newline character in _bytesRead as well
								size_t numberOfLineCharacters=((openedInBinaryMode?linefeedCharacterPosition+1:linefeedCharacterPosition)-_bytesRead->_chars->chars); // including the single quote at the start
								// adapt the length NOTE only in binary mode would there be a read 
								if(!openedInBinaryMode){
									readButNotInLine=_bytesRead->length-numberOfLineCharacters-1; // any characters between the NUL character and the linefeed character?????
									if(*(linefeedCharacterPosition-1)=='\r')numberOfLineCharacters--;
								}else
									readButNotInLine=_bytesRead->length-numberOfLineCharacters;
								// put the end-of-line marker where the line read ends
								_bytesRead->length=numberOfLineCharacters;*(_bytesRead->_chars->chars+numberOfLineCharacters)='\0';/////string_setlength(_bytesRead,numberOfLineCharacters);
							}else // everything actually read will need to be thrown back
								readButNotInLine=_bytesRead->length-1;
							// we'll throw back what wasn't consumed
							if(readButNotInLine>0){fseeko(file->_f,-readButNotInLine,SEEK_CUR);output("Moving the file cursor back %lld positions.\n",readButNotInLine);}
							if(NULL==linefeedCharacterPosition){FREE_STRING(_bytesRead,owner);_bytesRead=NULL;}
							/* replacing:
							// _bytesRead->_chars is a pointer to a Mchars which basically is simply a char array
							// of course getline() will allocate memory for the characters which might not be a multiple
							// of what blocks would do!!!
							// TODO can we write the characters read directly into _bytesRead->_chars????
							char* _lineRead=NULL;
							size_t len=0;
							ssize_t nread;
							nread=getline(&_lineRead,&len,file->_f);
							bool failed=true;
							if(nread!=-1){
								output("Number of characters read: %d.\n",len);
								// we have to remove the end-of-line characters
								// this means we have to look for '\n'
								while(len-->0&&_lineRead[len]!='\n');
								if(len>0||_lineRead[len]=='\n'){
									if(len>1&&_lineRead[len-1]=='\r')
										_lineRead[len-1]='\0';
									else
										_lineRead[len]='\0';
								}
								///* replacing:
								//char* p=(_lineRead+len);
								//while(len>0&&!*p){len--;p--;}; // skip all '\0'
								//while(len-->0){
								//	p--;
								//	if(*p!=13&&*p!=10)break;
								//	*p='\0';
								//}
								if(string_append(_bytesRead,_lineRead)!=NULL)
									failed=false;
							}
							if(failed){
								FREE_STRING(_bytesRead,owner);_bytesRead=NULL;
							}
							free(_lineRead);
							*/
							/* replacing: reading the line one character at a time
							Mstring* p=_bytesRead;
							// the file could be empty to start with
							int c=0;
							while(1){
								c=fgetc(file->_f);
								if(feof(file->_f))break; // to be tested AFTER reading
								// MDH@27DEC2020: whatever is read, it must not be negative!!!!
								if(c<SCHAR_MIN||c>SCHAR_MAX){
									output("%sInvalid character code (%i) read from '%s'.\n",M_ERROR_PREFIX,c,string(file->_name));
									break;
								}
								// let me suggest to only stop on \n and remove the \r if there's one in front of the \n
								if(c=='\n'){
									if(string_last_char(p)=='\r')string_declength(p);
									break; // either LF or CR would stop the reading
								}
								p=string_append_char(p,c);
								if(NULL==p){outputError("Failed to read all text line characters");break;}
							}
							///* MDH@30APR2024: since we didn't break on '\r' there's no need to actually do the following anymore
							//// skip the optional linefeed following any carriage return, if something else push back again
							//if(c=='\r')if(!feof(_file->_f)){c=fgetc(_file->_f);if(c!='\n')ungetc(c,_file->_f);}
							if(NULL==p){FREE_STRING(_bytesRead,owner);return NULL;}
							*/
						}
					}
				/*}else
					output("%sCan't read a line from file '%s' in binary mode.\n",M_ERROR_PREFIX,string(file->_name));*/
			}else
				output("%sCan't read a line from file '%s': it is not readable.\n",M_ERROR_PREFIX,string(file->_name));
		}else
			output("Can't read a line from file '%s': it is not open.\n",M_ERROR_PREFIX,string(file->_name));
		/* not here!!
		// if the file is disowned (which it will be if it was created)
		if(Misdisowned(file)){
			outputWarning("Reading a single line of text from a locally created file like this will always return the first text line!");
			free_file_file); // MDH@28DEC2020: will take care of closing the file as well now
		}
		return result;
		*/
	}
	return disowned_string(_bytesRead,owner);
}
/**
 * @brief returns a list containing the lines read from \p file
 * 
 * @param file 
 * @return Mlist* the list of lines read from \p file
 */
long long fReadLines(Mfile const * const file/*,Mallocationowner owner_file*/,long long numberOfLines,Mlist* linesReadList,Mallocationowner owner_linesReadList,bool report){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if((numberOfLines==M_LL_INVALID||numberOfLines>=0)&&file!=NULL&&linesReadList!=NULL){
		if(file->_f!=NULL){
			if(fIsReadable(file,false)){
				// before checking the mode to see if the file can be read from, we might need to open it
				// it's easiest to check whether it exists to start with, because if it doesn't it can't be read from anyway
				// if the file wasn't opened before, try to open it for reading 'text' (i.e. not binary)
				///////////if(NULL==file->_f)openFile(file,owner_file,"r+");
				// if the file can be read from, we do
				bool openedInBinaryMode=(fIsOpenedInBinaryMode(file)==M_TRUE);
				///////if(file->_mode[1]!='b'&&(strlen(file->_mode)<3||file->_mode[2]!='b')){ // the mode is defined (i.e. unequal to it's initial value '\0')
					result=0; // keep track of the number of lines read
					if(numberOfLines!=0){ // either all or some lines to read
						/* see Mfreadlines which should do the communication with the user
						if(numberOfLines!=M_LL_INVALID)
							output("About to read %lld lines from file '%s'.\n",numberOfLines,string(file->_name));
						else
							output("About to read all lines from file '%s'.\n",string(file->_name));
						*/
						if(!feof(file->_f)){ // MDH@27DEC2020 appended: to ascertain that NULL is returned
							/*
							_linesReadList=owned_list(_getListOfType(VT_TEXT),owner);
							if(_linesReadList!=NULL){*/
							/*
							char buffer[256]; // the buffer to use with fgets
							size_t buffer_length;
							bool eoln;
							*/
							// we'll be using a single line to read the individual lines into
							// NOTE when reading in binary mode, we can't actually reuse the line we use because we're actually storing the Mstring explicitly
							Mstring* _line=owned_string((openedInBinaryMode?__string():_getString("'")),owner);
							if(NULL==_line){outputError("Failed to create a line buffer");return M_LL_INVALID;}
							Mlistelement* listelement=NULL;
							/////output("Will start reading text lines.\n");
							// MDH@27MAY2024: for an example of how to handle reading a single line see fReadline()
							long long errorCode,readButNotStored,linesLeftToRead=numberOfLines;
							unsigned char* linefeedCharacterPosition=NULL;
							bool endOfFileReached=false;
							while(result>=0&&linesLeftToRead!=0&&!endOfFileReached){ // still text available and lines to read requested
								// STEP 1. TRY TO READ A TEXT LINE
								errorCode=string_freadline(_line,file->_f,&linefeedCharacterPosition);
								if(errorCode!=0)
									reportFileReadErrorCode(file,errorCode);
								/*else
									output("\tLine '%s' read successfully.\n",string(_line));*/
								endOfFileReached=feof(file->_f); // remember whether end-of-file was reached
								// MDH@27DEC2020: 
								// by appending a list element (with value NULL) we can tell whether
								// reading the file failed afterwards (if the last list element is NULL)
								// if storing the line text fails, listelement must be NULLed so that the
								// entire list will be freed (and nothing is returned)
								/////output("Reading the next line.\n");
								/* replacing:
								/////output("Copy at end: %lld.\n",copyAtEnd);
								if(numberOfCharsRead==M_LL_INVALID){ // something went wrong
									output("%sFailed to allocate memory to store text read from file '%s'.\n",M_ERROR_PREFIX,string(file->_name));
									result=-result;
									break;
								}
								if(numberOfCharsRead==0){
									result=-result;
									output("%sNo characters read from file '%s'.\n",M_ERROR_PREFIX,string(file->_name));
									break;
								}
								*/
								///output("Contents after reading %lld characters: '",_line->length-1);string_outputchars(_line,false);output("'\n");
								// TODO what should we do when numberOfCharsRead is negative?????
								// DONE see before the end of the loop where we're breaking out of this loop when that happens
								// PROCESS ALL TERMINATED LINES
								// NOTE if we fail to store we should treat the not registered characters read as REMAINDER
								unsigned char* lastStoredCharacterPosition=_line->_chars->chars; // the beginning of any line of characters read is where the single quote is located
								if(NULL==linefeedCharacterPosition&&endOfFileReached)linefeedCharacterPosition=lastStoredCharacterPosition+_line->length; // consume all characters read as last line
								if(openedInBinaryMode){ // opened in binary mode
									// we'll be storing __line as a whole, and 'return' the remainder read to the open file since the remainder may contain NUL characters, so we can't rely on moving the remainder
									// throw back what we're not going to store
									if(NULL==linefeedCharacterPosition){
										outputError("Failed to read a line");
										break;
									}
									long long lineLength=(linefeedCharacterPosition-lastStoredCharacterPosition+1);
									output("Length of line read: %lld.\n",lineLength);
									if(!endOfFileReached){ // only when the end of file is not yet reached do we reset the file cursor
										if(lineLength<_line->length){
											fseeko(file->_f,lineLength-_line->length,SEEK_CUR);
											output("File cursor moved back %lld positions.\n",_line->length-lineLength);
										}
									}
									_line->length=lineLength;
									listelement=getAppendedListelement(linesReadList,owner_linesReadList);
									if(NULL==listelement){
										result=-result;
										outputError("Failed to store the binary line read");
										break;
									}
									assignValue(&listelement->_value,_getStringValue(disowned_string(_line,owner)));
									if(NULL==listelement->_value){outputError("Failed to store a binary line read!");break;}
									// ASSERT _line is now bound in listelement->_value
									_line=owned_string(__string(),owner);
									if(NULL==_line){outputError("Failed to create a new binary buffer");break;}
								}else{ // opened in text mode
									readButNotStored=0; // for safety, just in case some positive value was still around
										// consume ALL text lines read
									unsigned char characterNulled='\0';
									while(linefeedCharacterPosition!=NULL){
										// all characters from startOfLine to linefeedCharacterPosition belong to the line to register
										// STEP 2. collect the text line
										if(linesLeftToRead>0)linesLeftToRead--; // one line less left to read
										result++; // another line read
										// register this line
										listelement=getAppendedListelement(linesReadList,owner_linesReadList);
										if(NULL==listelement){
											result=-result;
											outputError("Failed to store the text line read");
											break;
										}
										// STEP 1. mark the end of the text line NOTE this won't change the actual length, but will ascertain that _getTextValue() copies the actual line
										bool allCharactersProcessed=(*linefeedCharacterPosition=='\0');
										characterNulled='\0';
										/*
										if(openedInBinaryMode){ // opened in binary mode, we'll be storing the end-of-line characters in the text as well
											// remember the character we have to NULled
											if(!allCharactersProcessed){
												characterNulled=*(linefeedCharacterPosition+1);
												if(characterNulled)*(linefeedCharacterPosition+1)='\0';
											}
										}else*/
											if(*(linefeedCharacterPosition-1)=='\r')*(linefeedCharacterPosition-1)='\0';else *linefeedCharacterPosition='\0';
										// here we have the problem of NUL characters in the read text line
										assignValue(&listelement->_value,_getTextValue(lastStoredCharacterPosition));
										if(report)
											output("Line '%s' stored.\n",lastStoredCharacterPosition);
										// if some character was nulled write it back
										if(characterNulled)*(linefeedCharacterPosition+1)=characterNulled;
										if(allCharactersProcessed){output("All characters processed.\n");break;}
										/////////if(*linefeedCharacterPosition=='\0')break; // if this was adding the empty line at end-of-file (see below for how to force that), we allow doing that only once!!!
										// STEP 3. replace the end-of-line character with a single quote (') so we can use it as the new start of line position
										lastStoredCharacterPosition=linefeedCharacterPosition;
										// before replacing the linefeed character by a single quote we can check whether it already is because when it is this is the empty line at end-of-file we just added
										*lastStoredCharacterPosition='\''; // mark start of next line to process with a single quote
										// MDH@28MAY2024: there's a special situation where we should not break this is when we reached end-of-file right after a newline character
										//                in that situation we should add an empty line, this is when we now have ' followed by a NUL character
										if(endOfFileReached){
											// if we reached the end of the file, right behind a newline character (and nothing behind it), we force append a blank line once
											if(*(lastStoredCharacterPosition+1)=='\0'){linefeedCharacterPosition=lastStoredCharacterPosition+1;continue;}
											// OOPS you should never break on end-of-file as long as there are lines to process!!!!!!
											////////////break; // all read text now processed
										}
										if(linesLeftToRead==0)break; // stop as soon as we do not want to read another line, or we've processed all
										// find the next end-of-line character (if any), or bumping into the NUL character at the end in _line
										linefeedCharacterPosition=(unsigned char*)strchr(lastStoredCharacterPosition+1,'\n');
									}
									//// ALWAYS PROCESS ANY REMAINDER!!!!! if(result<0)break; // some error in storing the line read in _linesReadList
									// NOTE all full text lines in the characters read from the file processed
									// PROCESS THE REMAINDER
									// NOTE which we know does not end with a newline character
									//      which starts at startOfLine+1 up until where the line ends
									// MDH@28MAY2024: endOfFileReached could be true because all remaining characters were read although we may not have processed all of them
									//                therefore we should NOT set readButNotStored to 0 when endOfFileReached equals true
									readButNotStored=/*endOfFileReached?0:*/(_line->_chars->chars+_line->length)-(lastStoredCharacterPosition+1)/*)*/;
									if(result>=0){
										if(readButNotStored>0){ // not all characters stored WHICH now also means that we haven't reached end-of-file
											/////output("Read but not stored (yet): %lld.\n",readButNotStored);
											///////output("Read but not in line: %lld.\n",readButNotStoredYet);
											// if we stop reading now we have to give it back to the file, unless we're at end-of-file
											// no, we still have to give it back if we're currently at end-of-file if we can't store it
											if(linesLeftToRead!=0){ // NOT DONE we want to read more, so the remainder could still be completed
												///output("Trying to consume remainder.\n");
												/* MDH@27MAY2024: we've handled end-of-file
												// it is possible that there's nothing more to read from the file in which case we can simply attempt to store what we have
												if(feof(file->_f)){ // DONE but we can't because there's nothing left to read
													///output("End-of-file reached.\n");
													result++;
													listelement=getAppendedListelement(linesReadList,owner_linesReadList);
													if(NULL==listelement){
														result=-result;
														outputError("Failed to store the last text line");
													}else{
														assignValue(&listelement->_value,_getTextValue(string(startOfLine)));
														readButNotStored=0; // stored, so no need to actually return which means we're staying at end-of-file
														if(report)output("Line '%s' stored.\n",string(_line));
													}
													break; // we would be breaking anyway
												}
												*/
												///output("Moving the remainder to the start.\n");
												// ASSERT not at end-of-file and still lines to read so readButNotStored is completable
												long long numberOfCharsMoved=string_characters_moved(_line,(lastStoredCharacterPosition+1)-_line->_chars->chars,1);
												//////output("Number of characters moved: %lld.\n",numberOfCharsMoved);
												if(numberOfCharsMoved!=readButNotStored){ // not all characters moved, which means something went wrong, and we have to abort and give the remainder back!!!
													result=-result;
													output("%sOnly %lld out of %lld characters moved.\n",M_ERROR_PREFIX,numberOfCharsMoved,readButNotStored);
													break;
												}
												///////readButNotStored=0; // successfully moved, but since we're not breaking out of the loop yet, it doesn't matter
											}
										}else{
											_line->length=1;_line->_chars->chars[1]='\0';
										}
									}
									/* replacing:
									if(linesLeftToRead!=0){ // not done yet
										if(readButNotStored>0){
											long long numberOfCharsMoved=string_move(_line,(startOfLine+1)-_line->_chars->chars,1L);
											if(numberOfCharsMoved==M_LL_INVALID){
												if(result>0)result=-result;
												outputError("Failed to prepare for reading the next line(s)");
												break;
											}
											if(feof(file->_f)){ // nothing to read left from the file
												////output("Last line: '%s'.\n",string(_line));
												// augment the current remainder as well as the last line in the file which we haven't added yet
												// register this line
												/////// breaking anyway!!!! if(linesLeftToRead>0)linesLeftToRead--;
												result++;
												listelement=getAppendedListelement(linesReadList,owner_linesReadList);
												if(NULL==listelement){
													if(result>0)result=-result;
													outputError("Failed to append a text line list element");
												}else{
													readButNotStored=0; // do NOT allow the cursor to be reset
													assignValue(&listelement->_value,_getTextValue(string(_line)));
													if(report)output("Line '%s' stored.\n",string(_line));
												}
												break;
											}
										}else{
											_line->length=1;_line->_chars->chars[1]='\0';
										}
									}
									*/
									////////output("\tAfter update: '");string_outputchars(_line,false);output("'\n");
									///////if(report)output("Line '%s' read.\n",string(_line));
									// copy the part of the next line
									/* replacing
									eoln=false;
									char* newbuffer;
									// keep reading until all of the line is read (or some error occurs)
									do{
										buffer[0]='\0'; // ascertain for the buffer to have length 0 when we start
										newbuffer=fgets(buffer,256,file->_f);
										// detect read error
										if(NULL==newbuffer&&!feof(file->_f)){
											FREE_STRING(_line,owner);
											_line=NULL;
											output("%sSome error reading text from '%s'.\n",M_ERROR_PREFIX,string(file->_name));
										}else{
											// the problem is that buffer might not end with a new line
											buffer_length=strlen(buffer);
											if(buffer_length==0)break; // if nothing was read we're done (technically won't happen as fgets will also append the end of line!!!)
											buffer_length--;
											if(buffer[buffer_length]=='\n'){ // eoln
												buffer[buffer_length]='\0';
												eoln=true;
											}
											if(NULL==newbuffer)eoln=true; // we do want to add the last buffer
											if(NULL==string_append(_line,buffer)){ // error appending the buffer
												FREE_STRING(_line,owner);
												_line=NULL;
											}else{
												if(report)
													output("Buffer '%s' appended to '%s'.\n",buffer,string(_line));
												// if eoln the current line should be considered complete
												if(eoln)break;
											}
										}
									}while(_line!=NULL&&newbuffer!=NULL&&--numberOfLines>0); // MDH@30APR2024: testing for numberOfLines still not equal to zero!!!
									if(NULL==_line)break;
									// register this line
									assignValue(&listelement->_value,_getTextValue(string(_line)));
									*/
								}
								if(errorCode>0){if(result>0)result=-result;outputError("Some error occurred");break;}
							}
							// the part that was read but somehow not stored needs to be available for successive reading!!!
							if(!openedInBinaryMode){
								if(readButNotStored>0){
									output("Moving the file cursor %lld positions back.\n",readButNotStored);
									fseeko(file->_f,-readButNotStored,SEEK_CUR);
								}
								FREE_STRING(_line,owner);
							}
							// if we have a listelement we succeeded, well mostly
							/////if(listelement==NULL){FREE_LIST(_linesReadList,owner);return NULL;}
						}else
							output("%sCan't read from file '%s': end-of-file reached.",M_ERROR_PREFIX,string(file->_name));
					}else
						output("%sNo text lines can be read from '%s' (mode: %s): end-of-file reached.\n",M_WARNING_PREFIX,string(file->_name),file->_mode);
				/*}else
					output("%sUnable to read text lines from '%s' (mode: %s).\n",M_ERROR_PREFIX,string(file->_name),file->_mode);*/
			}else
				output("%sCan't read text lines from file '%s': it is not readable.\n",M_ERROR_PREFIX,string(file->_name));
		}else
			output("Can't read text lines from file '%s': it is not open.\n",M_ERROR_PREFIX,string(file->_name));
	}else
	if(NULL==file)
		outputError("No file to read from specified");
	else
		outputError("The number of lines to read is invalid");
	return result;
}

/**
 * @brief writes \p count bytes from \p bytes to file \p file
 * 
 * @param file the file (handle) to write to
 * @param bytes the bytes to write
 * @param count the number of bytes to write
 * @return long long the number of bytes not written
 */
static long long fWriteBytes(FILE * const file,unsigned char* bytes,size_t count){
	return(file!=NULL&&bytes!=NULL?(count>0?count-fwrite(bytes,sizeof(unsigned char),count,file):0):M_LL_INVALID);
}

/**
 * @brief writes \p str to file \p file 
 * 
 * @param file 
 * @param str 
 * @return long long the number of characters not written
 */
static long long fWriteString(FILE * const file,Mstring const * const str){
	return(file!=NULL&&str!=NULL?fWriteBytes(file,string(str),string_length(str)):M_LL_INVALID);
	/* replacing:
	long long result=M_LL_INVALID;
	if(file!=NULL&&str!=NULL){
		result=str->length; // the number of characters to write
		if(result>0)
			result-=fwrite(string(str),sizeof(unsigned char),result,file); // subtract from result what was actually written to the file
	}
	return result;
	*/
}
/**
 * @brief writes \p chars to \p file
 * 
 * @param file 
 * @param chars 
 * @return long long the number of characters not written, or M_LL_INVALID on invalid input
 */
static long long fWriteChars(FILE * const file,char const * const chars){
	long long result=M_LL_INVALID;
	if(file!=NULL&&chars!=NULL){
		result=strlen(chars);
		if(result>0){
			//////output("Writing %lld characters.\n",result);
			result-=fwrite(chars,sizeof(char),result,file);
		}
	}
	return result;
}
/**
 * @brief writes the C string in \p chars to file \p file
 * 
 * @param file 
 * @param chars 
 * @return long long the number of characters not written, or M_LL_INVALID when the input is incorrect (e.g. when we can't write to the ouput file)
 */
long long fWriteCharsToFile(Mfile const * const file/*,Mallocationowner owner_file*/,char const * const chars,bool writeEoln){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(file!=NULL&&chars!=NULL){
		if(file->_f!=NULL){
			size_t charsToWrite=strlen(chars); // the number of characters to write
			//////size_t eolnToWrite=(writeEoln?strlen(FILE_EOLN):0);
			long long notwritten=0;
			// if there's actually nothing to write we assume success (and notwritten will remain 0)
			if(charsToWrite||writeEoln){ // something left to write
				////////////////if(NULL==file->_f)openFile(file,owner_file,"w+"); // TODO I guess opening explicitly for writing seems to be the right choice
				if(file->_mode[1]=='+'||file->_mode[0]=='a'||file->_mode[0]=='w'){ // the mode is defined (i.e. unequal to it's initial value '\0')
					if(file->_mode[1]!='b'){ // text write (i.e. as characters)
						// the end-of-line is to precede the text to write!!!!!
						if(charsToWrite){
							// in case there are escape sequences in the text, we need to resolve these which _getStringOfText() does
							Mstring* _textToWrite=(notwritten==0?owned_string(_getStringOfChars(chars,'\0'),owner):NULL); // NOTE: replaces the _getStringOfText() call as used in fWriteText()
							if(_textToWrite!=NULL){
								notwritten=fWriteString(file->_f,_textToWrite);
								/* replacing:
								////////output("Writing '%s' to file '%s'.\n",string(_textToWrite),string(file->_name));
								char* p=string(_textToWrite); // replacing in order to get the NUL character written in _textToWrite!!!: _textToWrite->_chars->chars;
								while(*p){if(fputc(*p,file->_f)==EOF)break;p++;}
								while(*p){notwritten++;p++;} // count what has not been written
								*/
								FREE_STRING(_textToWrite,owner); // MDH@29APR2024: BUG FIX should be here
							}else{
								if(!notwritten){
									notwritten=charsToWrite;
									output("%sFailed to write text '%s'.\n",M_ERROR_PREFIX,chars);
								}else
									notwritten+=charsToWrite;
							}
						}
						if(writeEoln){ // also some EOLN characters to write
							notwritten+=fWriteChars(file->_f,FILE_EOLN);
							/* replacing:
							///output("Writing the end-of-line characters to '%s'.\n",string(file->_name));
							char* p=FILE_EOLN;
							// if we're supposed to write an end of line we increment notwritten all the same if we have to break!!!
							while(*p){if(fputc(*p,file->_f)==EOF)break;p++;}
							while(*p){notwritten++;p++;} // NOTE count what has not been written
							*/
						}
					}else // binary write
						notwritten=fWriteBytes(file->_f,chars,charsToWrite); // replacing: charsToWrite-fwrite(chars,sizeof(char),charsToWrite,file->_f);
				}
			}
			result=notwritten;
			if(notwritten)output("%sFailed to write %lld characters to '%s': %lld.\n",M_ERROR_PREFIX,string(file->_name),notwritten);
		}else
			outputError("Can't write to an unopened file");
	}
	return result;
}
/**
 * @brief writes \p str to \p file
 * 
 * @param file 
 * @param str
 * @return long long the number of bytes not written to \p file
 */
long long fWriteBytesToFile(Mfile const * const file,Mstring const * const str,bool writeEoln){
	long long result=M_LL_INVALID;
	if(file!=NULL&&str!=NULL){
		if(file->_f!=NULL){
			size_t bytesToWrite=string_length(str); // the number of characters to write
			size_t eolnToWrite=(writeEoln?strlen(FILE_EOLN):0);
			result=0;
			// if there's actually nothing to write we assume success (and notwritten will remain 0)
			if(bytesToWrite||eolnToWrite){ // something left to write
				////////////////if(NULL==file->_f)openFile(file,owner_file,"w+"); // TODO I guess opening explicitly for writing seems to be the right choice
				if(file->_mode[1]=='+'||file->_mode[0]=='a'||file->_mode[0]=='w'){ // the mode is defined (i.e. unequal to it's initial value '\0')
					if(file->_mode[1]!='b'){ // text write (i.e. as chars)
						// the end-of-line is to precede the text to write!!!!!
						if(bytesToWrite){
							result=fWriteBytes(file->_f,str->_chars->chars,result);
							/* replacing:
							result=bytesToWrite;
							result-=fwrite(str->_chars->chars,sizeof(unsigned char),result,file->_f);
							*/
						}
						if(eolnToWrite){ // also some EOLN characters to write
							result+=fWriteChars(file->_f,FILE_EOLN);
							/* replacing:
							///output("Writing the end-of-line characters to '%s'.\n",string(file->_name));
							char* p=FILE_EOLN;
							// if we're supposed to write an end of line we increment notwritten all the same if we have to break!!!
							while(*p){if(fputc(*p,file->_f)==EOF)break;p++;}
							while(*p){result++;p++;} // NOTE count what has not been written
							*/
						}
					}else // binary write
						result=fWriteBytes(file->_f,str->_chars->chars,bytesToWrite); // replacing: bytesToWrite-fwrite(str->_chars->chars,sizeof(unsigned char),bytesToWrite,file->_f);
				}
			}
			if(result)output("%sFailed to write %lld characters to '%s': %lld.\n",M_ERROR_PREFIX,string(file->_name),result);
		}else
			outputError("Can't write to an unopened file");
	}
	return result;
}

/**
 * @brief writes \p textToWrite to file \p file
 * 
 * @param file
 * @param textToWrite 
 * @return long long the number of bytes not written
 */
long long fWriteTextToFile(Mfile const * const file,Mtext const * const textToWrite,bool writeEoln){Mallocationowner owner=getOwner(__LINE__); // returns amount of bytes not written
	long long notwritten=M_LL_INVALID;
	if(file!=NULL&&textToWrite!=NULL){ // something to write
		if(file->_f!=NULL){ // an opened file we can write to
			char* p=textToWrite->_c; // _c is an array so a pointer
			size_t charsToWrite=strlen(p); // the number of characters to write
			/////size_t eolnToWrite=(writeEoln?strlen(FILE_EOLN):0);
			// if there's actually nothing to write we assume success (and notwritten will remain 0)
			notwritten=0;
			if(charsToWrite||writeEoln){ // something left to write
				///////////if(NULL==file->_f)openFile(file,owner_file,"w+"); // TODO I guess opening explicitly for writing seems to be the right choice
				if(file->_mode[1]=='+'||file->_mode[2]=='+'||file->_mode[0]=='a'||file->_mode[0]=='w'){ // the mode is defined (i.e. unequal to it's initial value '\0')
					if(file->_mode[1]!='b'){ // text write (i.e. as characters)
						// the end-of-line is to precede the text to write!!!!!
						// in case there are escape sequences in the text, we need to resolve these which _getStringOfText() does
						if(charsToWrite){
							Mstring* _textToWrite=(notwritten==0?owned_string(_getStringOfText(textToWrite,true),owner):NULL);
							if(_textToWrite!=NULL){
								notwritten=fWriteString(file->_f,_textToWrite);
								/* replacing:
								////////output("Writing '%s' to file '%s'.\n",string(_textToWrite),string(file->_name));
								char* p=string(_textToWrite); // replacing: _textToWrite->_chars->chars; // TODO not certain about this DONE damn right because apparently the \0 is not written at the end (which we found out by writing the previous line!!!)
								while(*p){if(fputc(*p,file->_f)==EOF)break;p++;}
								while(*p){notwritten++;p++;} // NOTE that's one way of dealing with it
								*/
								FREE_STRING(_textToWrite,owner); // MDH@29APR2024: BUG FIX should be here
							}else{
								if(!notwritten){
									notwritten=charsToWrite;
									output("%sFailed to write text '%s'.\n",M_ERROR_PREFIX,string(_textToWrite));
								}else
									notwritten+=charsToWrite;
							}
						}
					}else // binary write
						notwritten=fWriteBytes(file->_f,p,charsToWrite); //charsToWrite-fwrite(p,sizeof(char),charsToWrite,file->_f);
					if(writeEoln){ // also some EOLN characters to write
						notwritten+=fWriteChars(file->_f,FILE_EOLN);
						/* replacing:
						///output("Writing the end-of-line characters to '%s'.\n",string(file->_name));
						char* p=FILE_EOLN;
						// if we're supposed to write an end of line we increment notwritten all the same if we have to break!!!
						while(*p){if(fputc(*p,file->_f)==EOF)break;p++;}
						while(*p){notwritten++;p++;} // NOTE count what has not been written
						*/
					}
					/*
					if(eolnToWrite>0){
						if(notwritten==0){
							char* p=FILE_EOLN;
							// if we're supposed to write an end of line we increment notwritten all the same if we have to break!!!
							while(*p){if(fputc(*p,file->_f)==EOF)break;}p++;}
							while(*p){notwritten++;p++;} // count what we didn't write
						}else // failed to write the end-of-line as well
							notwritten+=eolnToWrite;
					}*/
				}
			}
		}else
			outputError("Can't write to an unopened file");
	}
	return notwritten;
}
/*
long long fWriteLine(Mfile const * const file,Mstring * const towrite){ // return amount of bytes not written
	long long result=M_LL_INVALID;
	if(file!=NULL&&towrite!=NULL){
		result=M_TRUE;
		fwrite(file,string(towrite));
		fwrite(file,EOLN);
	}
	return result;
}
*/

/**
 * @brief writes the elements in list \p linesToWrite to file \p file
 * 
 * @param file 
 * @param linesToWrite 
 * @return long long 
 */
static long long fWriteLines(Mfile * const file,Mlist const * const linesToWrite){Mallocationowner owner=getOwner(__LINE__); // returns the number of lines not written
	long long result=M_LL_INVALID;
	if(file!=NULL){
		if(linesToWrite!=NULL){
			if(file->_f!=NULL){
				result=0;
				
				long long numberOfLinesToWrite=linesToWrite->numberOfElements;
				if(numberOfLinesToWrite>0)output("Number of lines to write: %lld.\n",numberOfLinesToWrite);
				
				Mlistelement* lineListelement=linesToWrite->_first;
				while(lineListelement!=NULL){
					if(result>=0)result++;else result--;
					if(lineListelement->_value!=NULL){
						// TODO what if the value to write is not text????
						if(lineListelement->_value->type==VT_TEXT){
							Mtext* _lineText=lineListelement->_value->value._text;
							//// replacing: Mstring* _lineText=owned_string(_getValueText(lineListelement->_value,true,true),owner);
							if(_lineText!=NULL){
								if(fWriteTextToFile(file,_lineText,file->linesWritten>0)!=0){
									if(result>0)result=-result;
									output("%sFailed to write line #%lld '%s'.\n",M_ERROR_PREFIX,(result>0?result:-result),_lineText->_c);
								}else
									file->linesWritten++;
							}else
								output("%sNo line to write at list index %llu.",M_ERROR_PREFIX,lineListelement->index);
						}else
						if(lineListelement->_value->type==VT_BYTES){
							if(fWriteBytesToFile(file,lineListelement->_value->value._string,file->linesWritten>0)!=0){
								if(result>0)result=-result;
								output("%sFailed to write line #%lld of bytes.\n",M_ERROR_PREFIX,(result>0?result:-result));
							}else
								file->linesWritten++;
						}else{
							// write dequoted
							Mstring* _lineToWrite=owned_string(_getValueText(lineListelement->_value,true,true),owner);
							if(_lineToWrite!=NULL){
								if(fWriteString(file,_lineToWrite)!=0){
									if(result>0)result=-result;
								}
								FREE_STRING(_lineToWrite,owner);
							}
						}
					}
					lineListelement=lineListelement->_next;
				}
			}else
				outputError("Can't write to an unopened file");
		}else
			outputError("Nothing to write");
	}else
		outputError("No file to write to");
	return result;
}

// MDH@15MAY2024: getting and setting the file position is problematic because we're assuming that the file position is integer
//                which isn't guaranteed with fpos_t objects and fpos_t is returned by getpos() and cannot be constructed by yourself
//                since we're assuming a file is simply a sequence of bytes (although containing ASCII character text for now)
//                we'd have to switch to using ftell/fseek but in the long long (64-bit) variants ftello64 and fseek64
//                TODO will ftello64/fseeko64 always be available???????
/**
 * @brief returns the current position in opened file \p file
 * @details returns M_LL_INVALID if \p file does not denote a file or is not open
 * @param file 
 * @return long long the current position in \p file
 */
long long fPosition(Mfile const * const file){
	if(file!=NULL&&file->_f!=NULL){
		off_t position=ftello(file->_f);
		if(position<0){
			output(M_ERROR_PREFIX);
			switch(errno){
				case EBADF:output("'The file descriptor is not valid'");break;
				case EOVERFLOW:output("'The current file offset cannot be represented correctly in an object with the specified return type'");break;
				case ESPIPE:output("'The file descriptor underlying stream is associated with a pipe or FIFO'");break;
				default:output("Unknown error");break;
			}
			output(" reading the file position of file '%s'.\n",string(file->_name));
		}
		// in case off_t is a larger integer data type than long long!!!
		if(position<=M_LL_MAX)return(long long)position;
		outputError("File position too large!");
	}
	return M_LL_INVALID;
}
long long fSetPosition(Mfile const * const file,long long newposition){
	// TODO if we're going to get the current position anyway why not 
	long long position=fPosition(file);
	if(position>=0){ // the file is apparently open
		// determine the absolute position to move to 
		if(newposition>position){ // moving up
			// we do not want to allow moving beyond the end of the file but we need to know what the end-of-file position is
			if(fseeko(file->_f,0L,SEEK_END)==0){ // managed to move to end-of-file
				off_t filesize=ftello(file->_f);
				if(newposition>filesize)newposition=filesize;
			}else{
				newposition=M_LL_INVALID;
				if(file->_name!=NULL)
					output("Failed to obtain the size of file '%s'.\n",M_ERROR_PREFIX,string(file->_name));
				else
					outputError("Failed to obtain the size of the file");
			}
		}else // moving back but relative if newposition is negative, but non-negative newposition values should be reachable!!
		if(newposition<0){
			if(position+newposition<0)
				newposition=0;
			else
				newposition+=position;
		}
	}else
		output("Failed to obtain the current file position in setting the file position to '%lld'.\n",M_ERROR_PREFIX,newposition);
	// when newposition is valid, and we succeed in setting the file position to newposition, position should be set to that newposition
	if(newposition>=0){
		if(fseeko(file->_f,(off_t)newposition,SEEK_SET)==0)position=newposition;else output("%sFailed to set the file position to '%lld'.\n",M_ERROR_PREFIX,newposition);
	}else
		output("%sNew file position '%lld' invalid.\n",M_ERROR_PREFIX,newposition);
	return position;
}
/**
 * @brief pushes the current file position in \p file calling fGetPos() and stores it
 * @details returns M_LL_INVALID when \p file is undefined or is not opened, or storing the current file position failed
 * @param file 
 * @return long long M_TRUE on success, M_FALSE on failure
 */
long long fPushPosition(Mfile * const file,Mallocationowner file_owner){Mallocationowner owner=getOwner(__LINE__);
	if(file!=NULL&&file->_f!=NULL){
		// we will need to allocate a new Mfileposition
		Mfileposition* _fileposition=(Mfileposition*)MALLOC_1(sizeof(Mfileposition),'P',owner);
		if(_fileposition!=NULL){
			_fileposition->next=file->_filepositionstack;
			if(fgetpos(file->_f,&_fileposition->fpos)==0){ // success
				file->_filepositionstack=OWNED(DISOWNED(_fileposition,owner),Msubowner(file_owner,1));
				return M_TRUE;
			}
			outputError("Failed to store the current file position");
			FREE_DISOWNED_1(_fileposition,'P',owner);
		}else
			outputError("Failed to create a file position object");
		return M_FALSE;
	}
	return M_LL_INVALID;
}
/**
 * @brief restores the file position to the last stored fle position of \p file
 * @details returns M_LL_INVALID when \p file is not an opened file or popping the file position fails
 * @param file 
 * @param file_owner 
 * @return long long M_TRUE on success, M_FALSE on failure
 */
long long fPopPosition(Mfile * const file,Mallocationowner file_owner){
	long long result=M_LL_INVALID;
	if(file!=NULL&&file->_f!=NULL){
		if(file->_filepositionstack!=NULL){
			//output("Restoring the file position.\n");
			result=(fsetpos(file->_f,&file->_filepositionstack->fpos)?M_FALSE:M_TRUE);
			if(result==M_FALSE)
				outputError("Failed to restore the file position, but popping the stored file position anway");
			//else output("File position restored.\n");
			Mfileposition* nextFileposition=file->_filepositionstack->next;
			FREE_DISOWNED_1(file->_filepositionstack,'P',file_owner);
			output("File position popped!\n");
			file->_filepositionstack=nextFileposition;
		}
	}
	return result;
}
/**
 * @brief sets the file position cursor of \p file to the start of the file
 * @details returns M_LL_INVALID when \p file does not denote an opened file, or positioning the file position cursor fails
 * @param file 
 * @return long long M_TRUE on success, M_FALSE on failure
 */
long long fJumpToStart(Mfile * const file){
	if(file!=NULL&&file->_f!=NULL){ // an open file
		// TODO how about using rewind()
		// DONE using rewind() now which will reset the file error (which fseeko() would not)
		//      no, rewind() is void and so we cannot know whether we succeeded
		if(fseeko(file->_f,0L,SEEK_SET)==0){
			/*
			// reset the lines written count when the file is being written to
			// TODO should we restart
			if(file->_mode[0]=='w')
				file->linesWritten=0;
			*/
			// replacing: return M_TRUE;
		}else
			outputError("Failed to move the file cursor to the start of the file");
		return fPosition(file); // replacing: return M_FALSE;
	}
	return M_LL_INVALID;
}
/**
 * @brief sets the file position cursor of \p file to the start of the file
 * @details returns M_LL_INVALID when \p file does not denote an opened file, or positioning the file position cursor fails
 * @param file 
 * @return long long M_TRUE on success, M_FALSE on failure
 */
long long fJumpToEnd(Mfile const * const file){
	if(file!=NULL&&file->_f!=NULL){
		if(fseeko(file->_f,0L,SEEK_END)!=0)
			outputError("Failed to move the file cursor to the end of the file");
		return fPosition(file);
	}
	return M_LL_INVALID;
}

// end file helper functions

/**
 * @brief returns a new M value wrapping the M file stored or represented by \p filenameValue
 * 
 * @param filenameValue 
 * @return Mvalue* a new M value wrapping the M file stored or represented by \p filenameValue
 */
Mvalue* Mnewfile(Mvalue const * const filenameValue){
	return(filenameValue!=NULL?(filenameValue->type==VT_FILE?filenameValue:_getValueOfFile(_getFileWithName(filenameValue))):NULL);
}
/**
 * @brief flushes \p fileValue when open, returns M_TRUE on sucess, M_FALSE otherwise
 * 
 * @param fileValue 
 * @return Mvalue* M_TRUE on success, M_FALSE otherwise or M_LL_INVALID when called on an unopened file
 */
Mvalue* Mfflush(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	Mfile* file=(fileValue!=NULL&&fileValue->type==VT_FILE?fileValue->value._file:NULL);
	if(file!=NULL){
		if(file->_f!=NULL){
			if(fflush(file->_f)){
				result=M_FALSE;
				outputError("Failed to flush the file");
			}else
				result=M_TRUE;
		}else
			outputError("Can't flush an unopened file");
	}else
		outputError("Argument to fflush() not of type file");
	return _getIntegerValue(result);
}

// things we can do with an Mfile
// delete a file
/**
 * @brief returns M boolean M_TRUE when the file hosted in \p file_value is delete, M_FALSE otherwise
 * @details returns M_LL_INVALID if \p file_value does not host an existing file
 * @param file_value 
 * @return Mvalue* wrapping M_TRUE or M_FALSE
 */
Mvalue* Mfdelete(Mvalue* fileValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(fileValue!=NULL){
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		result=fDeleted(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner));
		if(_file!=NULL&&fileValue->type!=VT_FILE)FREE_FILE(_file,owner); // release the created file
	}
	return _getIntegerValue(result);
}
/**
 * @brief returns M_TRUE if \p fileValue represents a regular file, M_FALSE otherwise
 * 
 * @param fileValue 
 * @return Mvalue* M_TRUE if \p fileValue represents a regular file, M_FALSE otherwise
 */
Mvalue* Mfisfile(Mvalue const * const fileValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(fileValue!=NULL){
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		if(_file!=NULL){
			result=fIsRegularFile(_file,false);
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner); // release the created file
		}
	}
	return _getIntegerValue(result);
}
/**
 * @brief returns the file stat property map
 * 
 * @param fileValue 
 * @return Mvalue* the file stat property map
 */
Mvalue* Mfstat(Mvalue const * const fileValue){Mallocationowner owner=getOwner(__LINE__);
	if(fileValue!=NULL){
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		Mmap* _fileStatPropertyMap=owned_map(_getFileStatPropertyMap(_file),owner);
		if(_file!=NULL&&fileValue->type!=VT_FILE)FREE_FILE(_file,owner); // release the created file
		if(_fileStatPropertyMap!=NULL)
			return _getValueOfMap(disowned_map(_fileStatPropertyMap,owner));
	}
	return NULL;
}
/**
 * @brief returns the type of the file \p fileValue
 * 
 * @param fileValue 
 * @return Mvalue* the type of the file \p fileValue
 */
Mvalue* Mftype(Mvalue const * const fileValue){Mallocationowner owner=getOwner(__LINE__);
	if(fileValue!=NULL){
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		if(fIsRegularFile(_file,true)==M_TRUE){
			Mstring* _typetext=owned_string(_getString("'"),owner);
			bool success=false;
			if(_typetext!=NULL){
				fUpdateStats(_file,true);
				if(_file->staterrno==0){
					if(S_ISDIR(_file->stat.st_mode)){if(string_append(_typetext,"directory")!=NULL)success=true;}else
					if(S_ISCHR(_file->stat.st_mode)){if(string_append(_typetext,"character special file")!=NULL)success=true;}else
					if(S_ISBLK(_file->stat.st_mode)){if(string_append(_typetext,"block special file")!=NULL)success=true;}else
					if(S_ISREG(_file->stat.st_mode)){if(string_append(_typetext,"file")!=NULL)success=true;}else
					if(S_ISFIFO(_file->stat.st_mode)){if(string_append(_typetext,"FIFO special file/pipe")!=NULL)success=true;}else
					if(S_ISLNK(_file->stat.st_mode)){if(string_append(_typetext,"symbolic link")!=NULL)success=true;}else
					if(S_ISSOCK(_file->stat.st_mode)){if(string_append(_typetext,"socket")!=NULL)success=true;}
					else output("%sUnknown file type '%d'.\n",M_ERROR_PREFIX,_file->stat.st_mode);
				}
			}else
				outputError("Failed to create the file type text representation");
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner); // release the created file
			if(_typetext!=NULL){
				Mvalue* result=(success?_getValueOfText(_getText(string(_typetext))):NULL);
				FREE_STRING(_typetext,owner);
				if(result!=NULL)return result;
			}
		}else
		if(_file!=NULL)
			output("%sCan't determine the type of the file: '%s' is not an existing regular file.\n",M_ERROR_PREFIX,string(_file->_name));
		else
			outputError("No file specified");
	}
	return NULL;
}
/**
 * @brief returns the mode of \p fileValue
 * 
 * @param fileValue 
 * @return Mvalue* the mode of \p fileValue
 */
Mvalue* Mfmode(Mvalue* fileValue){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* result=NULL;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		Mstring* _modeText=owned_string(_getString("'"),owner);
		if(_modeText!=NULL){
			if(string_append(_modeText,fileValue->value._file->_mode)!=NULL)result=_getTextValue(string(_modeText));
			FREE_STRING(_modeText,owner);
		}
	}
	return result;
}
/**
 * @brief returns M_TRUE if \p fileValue is opened in binary file mode, M_FALSE or M_LL_INVALID otherwise
 * @details returns M_LL_INVALID if \p fileValue does not denote a file
 * @param fileValue 
 * @return Mvalue* M_TRUE if \p fileValue is opened in binary file mode, M_FALSE or M_LL_INVALID otherwise
 */
Mvalue* Mfisbinary(Mvalue* fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		result=fIsOpenedInBinaryMode(fileValue->value._file);
	}
	return _getIntegerValue(result);
}

/**
 * @brief returns information on opening the M file hosted in \p file_value in mode \p mode_value
 * 
 * @param file_value 
 * @param mode_value
 * @return Mvalue* the mode the file was opened in
 */
Mvalue* Mfopen(Mvalue* fileValue,Mvalue* openmodeTextValue){Mallocationowner owner=getOwner(__LINE__);
	// we need a valid open mode text value to start with
	Mfile* _file=(fileValue!=NULL?(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL)):NULL);
	if(_file!=NULL){ // there's a Mfile 
		if(NULL==_file->_f){ // an unopened file
			if(openmodeTextValue==NULL||openmodeTextValue->type==VT_TEXT){
				char* openmodeText=(openmodeTextValue!=NULL?openmodeTextValue->value._text->_c:NULL);
				// not anymore, fOpened will use whatever it can come up with!!! we allow a missing (NULL) openmode text but only when _file has a valid mode (from the previous opening as default)
				///if(openmodeText!=NULL||_file->_mode!=NULL){
					if(NULL==openmodeText||*openmodeText>=65){ // there's an open mode spec which at least is alphabetic
						char firstModeCharacter=(openmodeText!=NULL?tolower(*openmodeText):'\0');
						if(!firstModeCharacter||firstModeCharacter=='r'||firstModeCharacter=='w'||firstModeCharacter=='a'){
							char secondModeCharacter=(firstModeCharacter?tolower(*(openmodeText+1)):'\0');
							if(!secondModeCharacter||secondModeCharacter=='b'||secondModeCharacter=='+'){
								char thirdModeCharacter=(secondModeCharacter?tolower(*(openmodeText+2)):'\0');
								if(!thirdModeCharacter||thirdModeCharacter=='b'||thirdModeCharacter=='+'){
									if(!secondModeCharacter||!thirdModeCharacter||secondModeCharacter!=thirdModeCharacter){
										long long fileOpened=fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),openmodeText,false,false);
										if(fileOpened==M_TRUE)
											return(fileValue->type==VT_FILE?fileValue:_getValueOfFile(disowned_file(_file,owner)));
										if(openmodeText!=NULL)
											output("%sOpening the file in mode '%s' failed.\n",M_ERROR_PREFIX,openmodeText);
										else
											outputError("Failed to open the file in the default file mode");
											/*
											if(_file->mode[0])string_append_char(mode_str,_file->mode[0]);
											if(_file->mode[1])string_append_char(mode_str,_file->mode[1]);
											if(_file->mode[2])string_append_char(mode_str,_file->mode[2]);
											*/
										// opening failed, so if we created the file from the file's name, we have to free it
										if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner); // free the file we created because it won't be returned Mvalue wrapped
									}else
										outputError("The second and third mode character should be different!");
								}else
									outputError("The third mode character should be either 'b' or '+'");
							}else
								outputError("The second mode character should be either 'b' or '+'");
						}else
							outputError("%sThe first mode character should be either 'r', 'w' or 'a'");
					}else
					if(openmodeText!=NULL)
						output("%sInvalid open mode '%s'.\n",M_ERROR_PREFIX,openmodeText);
					else
						outputError("Invalid open mode");
				///}else outputError("If there is no default file mode (from a previous file opening) a second argument representing the open mode is required");
			}else
				outputError("The mode argument should be of type text");
		}else
			output("%sFile '%s' is already open. Close it first before reopening it!\n",M_ERROR_PREFIX,string(_file->_name));
	}else
		outputError("The file argument should either of type file or text (denoting the filename)");
	return NULL;
}
/**
 * @brief equivalent of Mfopen with the main focus on opening a file by name by instead of Mfopen returns the opened file
 * 
 * @param filenameValue 
 * @param openmodeTextValue 
 * @return Mvalue* 
 */
Mvalue* Mopen(Mvalue* filenameValue,Mvalue* openmodeTextValue){Mallocationowner owner=getOwner(__LINE__);
	// TODO delegating to Mfopen is probably best
	return Mfopen(filenameValue,openmodeTextValue);
	/* replacing:
	Mvalue* result=NULL;
	if(filenameValue!=NULL&&(openmodeTextValue==NULL||openmodeTextValue->type==VT_TEXT)){
		char* openmodeSpec=(openmodeTextValue!=NULL?openmodeTextValue->value._text->_c:NULL);
		if(filenameValue->type==VT_FILE){ // unexpected but we can still open this file
			outputWarning("Calling open() is preferred over calling open() to open a file created with file()");
			// if we manage to open the file successfully we return filenameValue as is, otherwise we return NULL!!
			if(fOpened(filenameValue->value._file,getValueDataOwner(),openmodeSpec,false,false)==M_TRUE)
				return filenameValue;
			output("%sFailed to open file '%s'.\n",M_ERROR_PREFIX,string(filenameValue->value._file->_name));
		}else
		if(filenameValue->type==VT_TEXT){
			Mfile* _file=owned_file(_getFile(filenameValue->value._text->_c),owner);
			if(_file!=NULL){
				if(fOpened(_file,owner,openmodeSpec,false,false)==M_TRUE)
					return _getValueOfFile(disowned_file(_file,owner));
				output("%sFailed to open the file with name '%s'.\n",M_ERROR_PREFIX,filenameValue->value._text->_c);
				// release the file because it is not being wrapped 
				FREE_FILE(_file,owner);
			}else
				output("%sFailed to create a file with name '%s'.\n",M_ERROR_PREFIX,filenameValue->value._text->_c);
		}
	}else
		outputError("Invalid input to open()");
	return result;
	*/
}
/**
 * @brief returns M_TRUE if \p fileValue exists, M_FALSE otherwise
 * @ details returns M_LL_INVALID if \p fileValue does not denote a file (or filename)
 * @param fileValue 
 * @return Mvalue* M_TRUE if \p fileValue exists, M_FALSE otherwise
 */
Mvalue* Mfexists(Mvalue* fileValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(fileValue!=NULL){
		if(fileValue->type==VT_FILE){
			result=fExists(fileValue->value._file,true); // report because fexists() returns true or false
		}else
		if(fileValue->type==VT_TEXT){
			Mfile* _file=owned_file(_getFile(fileValue->value._text->_c),owner);
			if(_file!=NULL){
				result=fExists(_file,true); // report
				FREE_FILE(_file,owner);
			}
		}else
			outputError("Invalid input to fexists()");
	}else
		outputError("No input to fexists()");
	return _getIntegerValue(result);
}
/**
 * @brief returns the file with name \p filenameValue if it exists, NULL otherwise
 * 
 * @param filenameValue the name of the file
 * @return Mvalue* the file with name \p filenameValue if it exists, NULL otherwise
 */
Mvalue* Mexists(Mvalue* filenameValue){Mallocationowner owner=getOwner(__LINE__);
	if(filenameValue!=NULL){
		if(filenameValue->type==VT_FILE){
			if(fExists(filenameValue->value._file,true)==M_TRUE)
				return filenameValue;
		}else
		if(filenameValue->type==VT_TEXT){
			Mfile* _file=owned_file(_getFile(filenameValue->value._text->_c),owner);
			if(_file!=NULL){
				if(fExists(_file,true)==M_TRUE)
					return _getValueOfFile(disowned_file(_file,owner));
				// the file does not exist, so won't be returned wrapped, and therefore needs to be discarded immediately
				FREE_FILE(_file,owner);
			}else
				output("%sFailed to create a file with name '%s'.\n",M_ERROR_PREFIX,filenameValue->value._text->_c);
		}else
			outputError("Invalid input to fexists()");
	}else
		outputError("No input to fexists()");
	return NULL;
}

Mvalue* Misdir(Mvalue const * const filenameValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(filenameValue!=NULL){
		if(filenameValue->type==VT_TEXT){
			Mfile* _file=owned_file(_getFile(filenameValue->value._text->_c),owner);
			if(_file!=NULL){	
				result=(fIsDir(_file,false)?M_TRUE:M_FALSE);
				FREE_FILE(_file,owner);
			}else	
				outputError("Failed to create the temporary file object");
		}else
		if(filenameValue->type==VT_FILE)
			result=(fIsDir(filenameValue->value._file,false)?M_TRUE:M_FALSE);
		else
			outputError("Invalid input to the isdir() function");
	}
	return _getIntegerValue(result);	
}
Mvalue* Mfisdir(Mvalue const * const fileValue){
	return Misdir(fileValue);
}

/**
 * @brief returns M_TRUE if nothing can be read from \p fileValue anymore, M_FALSE otherwise
 * @details returns M_LL_INVALID when \p fileValue does not represent a file or a file that is not opened
 * @param fileValue 
 * @return Mvalue* M_TRUE if nothing can be read from \p fileValue anymore, M_FALSE otherwise
 */
Mvalue* Mfeof(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	// can only be tested on an opened file
	if(fileValue!=NULL&&fileValue->type==VT_FILE&&fileValue->value._file->_f!=NULL)
		result=(feof(fileValue->value._file->_f)?M_TRUE:M_FALSE);
	return _getIntegerValue(result);
}
/**
 * @brief returns M_TRUE if \p fileValue denotes an opened file, M_FALSE otherwise
 * 
 * @param fileValue 
 * @return Mvalue* M_TRUE if \p fileValue denotes an opened file, M_FALSE otherwise
 */
Mvalue* Mfisopen(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE)
		result=(fileValue->value._file->_f!=NULL?M_TRUE:M_FALSE);
	return _getIntegerValue(result);
}
/**
 * @brief return the size (in bytes) of \p fileValue
 * 
 * @param fileValue 
 * @return Mvalue* the size (in bytes) of \p fileValue
 */
Mvalue* Mfsize(Mvalue const * const fileValue){
	// if the file is open there's an easy way to do it
	long long result=M_LL_INVALID;
	if(fileValue!=NULL){
		if(fileValue->type==VT_FILE){
			// if the file is currently open it's pretty straight-forward
			Mfile* file=fileValue->value._file;
			if(file!=NULL){
				// if fPosition returns something unequal to M_LL_INVALID it is considered to be open currently
				long long filepos=fPosition(file);
				if(filepos!=M_LL_INVALID){ // treat the file as an opened file
					// TODO how to deal with files opened in binary mode????
					if(fseeko(file->_f,0L,SEEK_END)==0){ // move position to the end of the file
						result=fPosition(file); // determine the 
						if(fseeko(file->_f,filepos,SEEK_SET))outputError("Failed to return to the current position of the file");
					}else{
						filepos==M_LL_INVALID;
						outputError("Failed to move the file position to the end of the file to determine the file size");
					}
				}
				if(filepos==M_LL_INVALID){ // determining the file size from an opened file failed or it is not open to start with
					if(file->_f!=NULL)outputWarning("Failed to determine the current file position");
					bool fileExists=(fExists(file,false)==M_TRUE);
					if(fileExists&&fIsRegularFile(file,false)){ // we know that file->stat is now up to date and contains the size of the file
#if defined _WIN32 || defined _WIN64 || defined __WIN32 || defined _WCE || defined MSDOS || defined __MSDOS || defined OS2 || defined _OS2 || defined __OS2___
						long long filepos=fPosition(file);
						bool fileisopen=(filepos!=M_LL_INVALID);
						// TODO how to deal with files opened in binary mode????
						if(!fileisopen&&fOpened(file,getValueDataOwner(),"r",false,false)==M_TRUE)filepos=fPosition(file);
						if(filepos!=M_LL_INVALID){ // the current position is known
							if(fseeko(file->_f,0L,SEEK_END)==0){ // move position to the end of the file
								result=fPosition(file); // determine the 
								if(fseeko(file,filepos,SEEK_SET))outputError("Failed to return to the current position of the file");
							}else
								outputError("Failed to move to the end of the file");
						}
						if(filepos!=M_LL_INVALID)if(!fileisopen)if(fClosed(file,getValueDataOwner(),false)!=M_TRUE)outputError("Failed to close a temporarily opened file");
#else
						if(file->staterrno==0)
							result=file->stat.st_size; // easiest way to get the size of the file
						else
							output("Can't obtain the size of file '%s' because of error '%s'.\n",M_ERROR_PREFIX,string(file->_name),strerror(file->staterrno));
#endif
					}else
					if(fileExists)
						output("%sCan't determine the size of '%s': it is not a regular file.\n",M_ERROR_PREFIX,string(file->_name));
					else
						output("%sCan't get the size of file '%s': it does not exist!\n",M_ERROR_PREFIX,string(file->_name));
				}
			}else
				outputError("File argument undefined");
		}else
		if(fileValue->type==VT_TEXT){
			struct stat filestats;
			if(stat(fileValue->value._text->_c,&filestats)==0)
				result=filestats.st_size;
			else 
				output("%sError '%s' occurred accessing the status of file '%s'.\n",M_ERROR_PREFIX,strerror(errno),fileValue->value._text->_c);
		}else
			outputError("The argument should either be of type file or of type text (denoting the name of an existing file)");
	}
	return _getIntegerValue(result);
}

/*
// reading from a file by specifying the number of bytes to read
static void openFileForReadingText(Mfile* _file){
	// ASSERT _file should NOT be NULL and _file->_stat should not be NULL (i.e. it's an existing file) and _file->_f should be NULL
	// if the file mode has not been set yet, or if it has been set to something that is supposedly readable
	if(_file&&!_file->_f){ // defined, but not open yet
		if(_file->mode[0]=='\0'||_file->mode[0]!='w'||_file->mode[1]=='+'){
			_file->_f=fopen(string(_file->_name),(_file->mode[0]?_file->mode:"r+")); // an array is a pointer!!!!!
			if(_file->_f)if(!_file->mode[0]){_file->mode[0]='r';_file->mode[1]='+';} // if the file mode was not initialized, use r+ as access mode
		}
	}
}*/

static long long getOpenedFile(Mfile* file,char* mode){

}
// to allow for continued reading we should keep track of the position in the file?????? I guess we can keep a reference to the FILE pointer I suppose
/**
 * @brief returns at most the number of bytes hosted in \p numberofbytes_value bytes read from the M file hosted in \p file_value
 * @details reads the bytes one at a time using fgetc()
 * @param file_value 
 * @param numberofbytes_value 
 * @return Mvalue* the new M string containing the read bytes
 */
Mvalue* Mfread(Mvalue* fileValue,Mvalue* numberOfBytesValue){Mallocationowner owner=getOwner(__LINE__);
	Mvalue* result=NULL;
	long long numberOfBytes=(numberOfBytesValue!=NULL?getValueInteger(numberOfBytesValue):M_LL_INVALID); // the default is to read as much bytes as possible
	if(numberOfBytes>=0||numberOfBytes==M_LL_INVALID){ // only non-negative values are considered valid
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		if(_file!=NULL){
			bool opened=false;
			if(NULL==_file->_f){ // try to open the file to read from
				if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"r",false,false)==M_TRUE)
					opened=true;
				else
					output("%sFailed to open file '%s' to read from.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			if(_file->_f!=NULL){
				if(!feof(_file->_f)){
					// get the result of reading at most numberOfBytes bytes
					Mstring* _bytesRead=owned_string(fRead(_file,numberOfBytes),owner); // do not forget to take over the ownership
					if(_bytesRead!=NULL){ // a single quoted text!!!
						output("Number of bytes read: %lld.\n",string_length(_bytesRead));
						if(!fIsOpenedInBinaryMode(_file)){ // opened in 'text' mode
							result=_getStringTextValue(_bytesRead); // _getStringTextValue() escapes NUL characters!!!!! TODO this would copy what was read again, can we speed this up????
							FREE_STRING(_bytesRead,owner);
						}else // opened in binary mode
							result=_getStringValue(disowned_string(_bytesRead,owner));
					}else
						output("%sCan't read from file '%s': not enough memory available!\n",M_ERROR_PREFIX,string(_file->_name));
				}else
					output("%sCan't read from file '%s': end-of-file reached!\n",M_ERROR_PREFIX,string(_file->_name));
				// close the file if opened here
				if(opened)closeFile(_file,true);
			}
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
			///////else if(opened&&_file->_f!=NULL&&!closeFile(_file,true))output("%sFailed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
		}else
			outputError("Either a file or a file name required as first argument");
	}else
			outputError("No file to read from");
	return result; // some error
}
/**
 * @brief returns a new M string value containing a full text line read from the M file hosted in \p fileValue 
 * 
 * @param fileValue 
 * @return Mvalue* a full text line M styring value read from the M file hosted in \p fileValue
 */
Mvalue* Mfreadline(Mvalue* fileValue){Mallocationowner owner=getOwner(__LINE__); // reads all bytes until a new line character is encountered 
	// MDH@27DEC2020: file_value really needs to be a file, although theoretically one
	//				might want to read the first line of a file only???????
	//				for now let's allow both although using the filename is not recommended
	Mvalue* result=NULL;
	if(fileValue!=NULL){
		Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
		if(_file!=NULL){
			bool opened=false;
			if(NULL==_file->_f){ // try to open the file to read from
				if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"r",false,false)==M_TRUE)
					opened=true;
				else
					output("%sFailed to open file '%s' to read from.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			if(_file->_f!=NULL){
				if(!feof(_file->_f)){
					/////output("Reading a line from '%s'.\n",string(_file->_name));
					Mstring* _textLineRead=owned_string(fReadLine(_file),owner);
					if(_textLineRead!=NULL){ // a single quoted string
						output("Line read: '%s'.\n",string(_textLineRead));
						if(fIsOpenedInBinaryMode(_file)!=M_TRUE){
							result=_getStringTextValue(_textLineRead);
							FREE_STRING(_textLineRead,owner);
						}else
							result=_getStringValue(disowned_string(_textLineRead,owner)); // an immutable version of the bytes obtained
					}else
						output("%sFailed to obtain memory to read a text line from '%s' into.\n",M_ERROR_PREFIX,string(_file->_name));
					if(opened)closeFile(_file,true);
				}else
					output("%sCan't read a text line from '%s': end-of-file reached.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			// free the file when it was created
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
			//else if(opened&&_file->_f!=NULL&&!closeFile(_file,true))output("Failed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
		}else
			outputError("Either a file or a file name required as first argument");
	}else
		outputError("No file (name) to read from specified");
	return result;
}
// MDH@01OCT2020: how about allowing to read a number of lines in one go??????
// MDH@27DEC2020: if the list returned ends with NULL we failed
/**
 * @brief reads at most \p numberOfLinesValue lines from \p fileValue and appends them to the optional \p listValue
 * 
 * @param fileValue 
 * @param numberOfLinesValue 
 * @param listValue
 * @return Mvalue* the list \p listValue or a new list with the lines that were read from \p fileValue appended
 */
Mvalue* Mfreadlines(Mvalue* fileValue,Mvalue* numberOfLinesValue,Mvalue* listValue){Mallocationowner owner=getOwner(__LINE__);
	bool report=(amVerboseDebugging()||(M_MODULE_DEBUGGING&MM_VALUE));
	long long numberOfLines=(numberOfLinesValue!=NULL?getValueInteger(numberOfLinesValue):M_LL_INVALID);
	Mvalue* result=NULL;
	if(numberOfLines==M_LL_INVALID||numberOfLines>=0){
		if(fileValue!=NULL){
			Mfile* _file=(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL));
			if(_file!=NULL){ // we've got a file to read from
				bool opened=false;
				if(NULL==_file->_f){ // try to open the file to read from
					if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"r",false,false)==M_TRUE)
						opened=true;
					else
						output("%sFailed to open file '%s' to read from.\n",M_ERROR_PREFIX,string(_file->_name));
				}
				if(_file->_f!=NULL){
					if(!feof(_file->_f)){ // end-of-file not reached yet!!
						if(numberOfLines==M_LL_INVALID)
							output("About to read all lines from file '%s'.\n",string(_file->_name));
						else
							output("About to read %lld lines from file '%s'.\n",numberOfLines,string(_file->_name));
						long long filepos=fPosition(_file); // MDH@08MAY2024: we'll need the current file position to know whether to close the file again or not
						if(filepos<0)outputWarning("Failed to obtain the current file position");
						Mlist* textLinesReadBefore=NULL;
						if(listValue!=NULL){
							if(listValue->type!=VT_LIST){
								outputValue("Third argument to fReadlines() (",listValue,") not a list.\n");
							}else
								textLinesReadBefore=listValue->value._list; 
						}
						Mlist* _textLinesRead=(textLinesReadBefore==NULL?owned_list(__list("fReadlines"),owner):textLinesReadBefore);
						if(_textLinesRead!=NULL){
							///long long linesReadBefore=_textLinesRead->numberOfElements;
							long long numberOfLinesRead=fReadLines(_file,numberOfLines,_textLinesRead,(textLinesReadBefore!=NULL?getValueDataOwner():owner),report);
							if(numberOfLinesRead>=0){
								if(numberOfLines>=0&&numberOfLinesRead!=numberOfLines)
									output("%sOnly %lld out of the requested %lld lines read from file '%s'.\n",M_WARNING_PREFIX,numberOfLinesRead,numberOfLines,string(_file->_name));
								// either returning listValue or the new created list
								result=_getValueOfList(textLinesReadBefore!=NULL?listValue:disowned_list(_textLinesRead,owner));
								// if we've reached the end of the file and we've read ALL lines, we close the file as a service to the user
								if(fileValue->type==VT_FILE&&feof(_file->_f)){
									if(filepos==0){
										if(closeFile(_file,false)){
											opened=false;
											output("File '%s' from which all text lines were read closed!\n",string(_file->_name));
										}else
											output("%sFailed to close file '%s'!",M_ERROR_PREFIX,string(_file->_name));
									}else
										output("%sFile '%s' not closed, although end-of-file reached.\n",M_WARNING_PREFIX,string(_file->_name));
								}
							}else{
								// free the created list as it will not be returned
								if(textLinesReadBefore==NULL)FREE_LIST(_textLinesRead,owner);
								output("%sFailed to read lines from '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
							}
						}else
							outputError("Failed to create the list to store the lines read into");
					}else
						output("%sCan't read from file '%s': end-of-file reached.\n",M_ERROR_PREFIX,string(_file->_name));
					if(opened)closeFile(_file,true);
				}else
					output("%sCan't read from unopened file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
				if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
				/////// else if(opened&&_file->_f!=NULL&&!closeFile(_file,true))output("Failed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
			}else
				outputError("Either a file or a file name required as first argument");
		}else
			outputError("No file to read lines from");
	}else
		output("%sThe number of lines to read specified (%lld) in invalid!",M_ERROR_PREFIX,numberOfLines);
	return result;
}

/**
 * @brief closes the M file hosted in \p fileValue
 * 
 * @param fileValue 
 * @return Mvalue* M_TRUE on success, M_FALSE on failure, or M_LL_INVALID if \p fileValue does not host a file
 */
Mvalue* Mfclose(Mvalue* fileValue){
	Mfile* _file=(fileValue!=NULL&&fileValue->type==VT_FILE?fileValue->value._file:NULL);
	return _getIntegerValue(fClosed(_file,getValueDataOwner(),false)); // NOTE in the future we might not need to pass the owner to fClosed anymore
}

/**
 * @brief returns the current file position of the opened file wrapped in \p fileValue
 * @details returns M_LL_INVALID if no file is specified or the file is not open
 * @param fileValue the wrapper of the opened file
 * @return Mvalue* the current file position
 */
Mvalue* Mfpos(Mvalue const * const fileValue){
	Mfile* _file=(fileValue!=NULL&&fileValue->type==VT_FILE?fileValue->value._file:NULL);
	long long filepos=(long long)fPosition(_file);
	return _getIntegerValue(filepos);
}
/**
 * @brief sets the current position in file \p fileValue to \p newpositionValue
 * 
 * @param fileValue 
 * @param newpositionValue 
 * @return Mvalue* the current file position on sucess, M_LL_INVALID on failure
 */
Mvalue* Mfsetpos(Mvalue* fileValue,Mvalue* newpositionValue){
	long long result=M_LL_INVALID;
	if(newpositionValue!=NULL&&(newpositionValue->type==VT_INTEGER||newpositionValue->type==VT_BIGINTEGER)){
		Mfile* _file=(fileValue!=NULL&&fileValue->type==VT_FILE?fileValue->value._file:NULL);
		if(_file!=NULL&&_file->_f!=NULL){
			long long newposition=getValueInteger(newpositionValue); // TODO newpositionValue should be a valid integer somehow
			if(newposition!=M_LL_INVALID)result=(long long)fSetPosition(_file,newposition);
		}else
		if(_file!=NULL)
			outputError("No open file specified");
		else
			outputError("No file specified");
	}else
		outputError("No or an invalid position specified");
	return _getIntegerValue(result);
}
/**
 * @brief remembers the current file position in \p fileValue onto the file position stack
 * @details returns M_LL_INVALID when \p fileValue does not represent an opened file, or when remembering the file position fails
 * @param fileValue 
 * @return Mvalue* M_TRUE on success, M_FALSE on failure
 */
Mvalue* Mfpushpos(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		result=fPushPosition(fileValue->value._file,getValueDataOwner());
	}else
		outputError("The argument to fpushpos() does not denote an opened file");
	return _getIntegerValue(result);
}
Mvalue* Mfpoppos(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		result=fPopPosition(fileValue->value._file,getValueDataOwner());
	}else
		outputError("The argument to fpoppos() does not denote an opened file");
	return _getIntegerValue(result);
}
/**
 * @brief moves the file position in opened file \p fileValue to the start of the file
 * @details returns M_LL_INVALID when \p fileValue does not denote an opened file, or moving the file position to the start fails
 * @param fileValue 
 * @return Mvalue* M_TRUE on success, M_FALSE on failure
 */
Mvalue* Mftostart(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		result=fJumpToStart(fileValue->value._file);
	}else
		outputError("The argument to fpushpos() does not denote an opened file");
	return _getIntegerValue(result);
}
/**
 * @brief moves the file position in opened file \p fileValue to the end
 * @details returns M_LL_INVALID when \p fileValue does not denote an opened file, or moving the file position to the end fails
 * @param fileValue 
 * @return Mvalue* M_TRUE on success, M_FALSE on failure
 */
Mvalue* Mftoend(Mvalue const * const fileValue){
	long long result=M_LL_INVALID;
	if(fileValue!=NULL&&fileValue->type==VT_FILE){
		result=fJumpToEnd(fileValue->value._file);
	}else
		outputError("The argument to fpushpos() does not denote an opened file");
	return _getIntegerValue(result);
}
/**
 * @brief sets the file position in \p fileValue to \p positionValue
 * 
 * @param fileValue 
 * @param positionValue 
 * @return Mvalue* the current file position
 */
Mvalue* Mfseek(Mvalue* fileValue,Mvalue* newpositionValue){
	long long newposition=M_LL_INVALID;
	if(newpositionValue!=NULL){
		if(newpositionValue->type==VT_INTEGER)
			newposition=newpositionValue->value._integer->ll;
		else
		if(newpositionValue->type==VT_BIGINTEGER)
			newposition=getBigintegerInteger(newpositionValue->value._biginteger);
	}
	if(newposition!=M_LL_INVALID){
		Mfile* _file=(fileValue!=NULL&&fileValue->type==VT_FILE?fileValue->value._file:NULL);
		if(_file!=NULL)newposition=fSetPosition(_file,newposition);else outputError("File argument to fseek() not of type file");
	}else
		outputError("Invalid second (new file position) argument to fseek()");
	return _getIntegerValue(newposition);
}
/*
static void openFileForWriting(Mfile* _file){
	// ASSERT _file should NOT be NULL and _file->_f should be NULL (but _file->stat might be NULL)
	// if the file mode has not been set yet, or if it has been set to something that is supposedly readable
	if(_file->mode[0]=='\0'||_file->mode[0]!='w'||_file->mode[1]=='+'){
		_file->_f=fopen(string(_file->_name),(_file->mode[0]?_file->mode:"r+")); // an array is a pointer!!!!!
		if(_file->_f)if(!_file->mode[0]){_file->mode[0]='r';_file->mode[1]='+';} // if the file mode was not initialized, use r+ as access mode
	}
}*/

/**
 * @brief writes \p valueToWrite to file \p file 
 * 
 * @param file the handle of the file to write to
 * @param valueToWrite the value to write 
 * @param binary whether or not the file is opened in binary (write) mode
 * @return the number of bytes not written to the output file, or M_LL_INVALID when the input is incorrect
 */
long long fWriteValue(FILE* const file,Mvalue* valueToWrite,bool openedInBinaryMode){static Mallocationowner owner=OWNER(__LINE__); // saves creating the same owner on every occasion
	long long result=M_LL_INVALID;
	if(file!=NULL&&valueToWrite!=NULL){
		if(openedInBinaryMode){
			if(valueToWrite->type==VT_LIST){
				result=0;
				Mlist* listToWrite=valueToWrite->value._list;
				if(listToWrite!=NULL){
					Mlistelement* listelementToWrite=listToWrite->_first;
					while(listelementToWrite!=NULL){
						if(listelementToWrite->_value!=NULL)
							result+=fWriteValue(file,listelementToWrite->_value,true);
						listelementToWrite=listelementToWrite->_next;
					}
				}
			}else
			if(valueToWrite->type==VT_ARRAY){
				result=0;
				Marray* array=valueToWrite->value._array;
				if(array!=NULL){
					Mvalue** values=array->values;
					long long numberOfWrites=array->numberOfElements;
					///////output("Number of array elements to write: %llu.\n",numberOfWrites);
					while(numberOfWrites>0){
						if(*values!=NULL)
							result+=fWriteValue(file,*values,true);
						/* replacing:
						if(result>=0)result++;else result--;
						if(arrayelementToWrite->type==VT_TEXT){
							result+=fWriteCharsToFile(_file,arrayelementToWrite->value._text->_c,false)!=0);
						}else
						if(arrayelementToWrite->type==VT_BYTES){
							result+=fWriteBytesToFile(_file,arrayelementToWrite->value._string,false)!=0)if(result>0)result=-result;
						}
						*/
						++values;
						--numberOfWrites;
					}
				}
			}else
			if(valueToWrite->type==VT_TEXT){
				result=fWriteChars(file,valueToWrite->value._text->_c);
				/* replacing:
				result=strlen(valueToWrite->value._text->_c);
				result-=fwrite(valueToWrite->value._text->_c,sizeof(unsigned char),result,file);
				*/
				// replacing:	result=(fWriteChars(_file,valueToWrite->value._text->_c,false)==0?M_TRUE:M_FALSE); // not to write an end-of-line
			}else
			if(valueToWrite->type==VT_BYTES){
				result=fWriteString(file,valueToWrite->value._string);
				// replacing: result=(fWriteBytesToFile(_file,valueToWrite->value._string,false)==0?M_TRUE:M_FALSE); // not to write an end-of-line
			}else{
				Mstring* _valueToWriteText=owned_string(_getValueText(valueToWrite,true,true),owner);
				if(_valueToWriteText!=NULL){
					result=fWriteString(file,_valueToWriteText);
					FREE_STRING(_valueToWriteText,owner);
				}else
					outputError("%sFailed to obtain the text representation to write to a file");
			}
		}else{ // writing to a text file
			Mstring* _valueToWriteText=owned_string(_getValueText(valueToWrite,true,true),owner);
			if(_valueToWriteText!=NULL){
				result=fWriteString(file,_valueToWriteText);
				FREE_STRING(_valueToWriteText,owner);
			}else
				outputError("%sFailed to obtain the text representation to write to a file");
		}
		/*
		if(binary){
			// lists and arrays need to be written in succession

		}else{
			// we need to write the text representation of value to the output file
			Mstring* valueText=owned_string(_getValueText(valueToWrite,false,false));
			if(_valueText!=NULL){

			}
		}
		*/
	}else
	if(NULL==valueToWrite)
		outputError("No value to write!");
	return result;
}

/**
 * @brief writes the text hosted in \p writeValue to the M file hosted in \p fileValue
 * 
 * @param fileValue 
 * @param writeValue 
 * @return Mvalue* the number of characters NOT written, M_LL_INVALID if writing failed
 */
Mvalue* Mfwrite(Mvalue* fileValue,Mvalue* writeValue){Mallocationowner owner=getOwner(__LINE__); // reads all bytes until a new line character is encountered 
	// how about returning the number of bytes NOT written...
	long long result=M_LL_INVALID;
	if(writeValue!=NULL){
		Mfile* _file=(fileValue!=NULL?(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL)):NULL);
		if(_file!=NULL){
			bool opened=false;
			if(NULL==_file->_f){
				// always opening in binary append mode (Mfwriteline would open in text append mode)
				if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"ab+",false,false)==M_TRUE)
					opened=true;
				else
					output("%sFailed to open file '%s' in binary append mode.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			if(_file->_f!=NULL){
				result=fWriteValue(_file->_f,writeValue,fIsOpenedInBinaryMode(_file)==M_TRUE);
				if(opened)closeFile(_file,true);
				/* replacing:
				if(fIsOpenedInBinaryMode(_file)!=M_TRUE){ // opened as text file, so we should write the text representation
					Mstring* _writeValueText=owned_string(_getValueText(writeValue,false,true),owner);
					if(_writeValueText!=NULL){
						result=fWriteString(_file->_f,_writeValueText);
						FREE_STRING(_writeValueText,owner);
					}else
						output("%sFailed to create the text to write to file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
				}else
					result=fWriteValueToBinaryFile(_file->_f,writeValue);
				*/
				/* replacing:
				if(writeValue->type==VT_TEXT||writeValue->type==VT_BYTES){
					if(writeValue->type==VT_TEXT)
						result=(fWriteChars(_file,writeValue->value._text->_c,false)==0?M_TRUE:M_FALSE); // not to write an end-of-line
					else
					if(writeValue->type==VT_BYTES)
						result=(fWriteBytesToFile(_file,writeValue->value._string,false)==0?M_TRUE:M_FALSE); // not to write an end-of-line
				}else
				if(writeValue->type==VT_LIST){
					result=0;
					Mlist* writes=writeValue->value._list;
					long long numberOfWrites=(writes!=NULL?writes->numberOfElements:0);
					if(numberOfWrites>0){
						output("Number of writes: %lld.\n",numberOfWrites);
						Mlistelement* lineListelement=writes->_first;
						while(lineListelement!=NULL){
							if(result>=0)result++;else result--;
							if(lineListelement->_value!=NULL){
								// TODO what if the value to write is not text????
								if(lineListelement->_value->type==VT_TEXT){
									Mtext* _lineText=lineListelement->_value->value._text;
									//// replacing: Mstring* _lineText=owned_string(_getValueText(lineListelement->_value,true,true),owner);
									if(_lineText!=NULL){
										if(fWriteTextToFile(_file,_lineText,false)!=0){
											if(result>0)result=-result;
											output("%sFailed to write line #%lld '%s'.\n",M_ERROR_PREFIX,(result>0?result:-result),_lineText->_c);
										}
									}else
										output("%sNo line to write at list index %llu.",M_ERROR_PREFIX,lineListelement->index);
								}else
								if(lineListelement->_value->type==VT_BYTES){
									if(fWriteBytesToFile(_file,lineListelement->_value->value._string,false)!=0){
										if(result>0)result=-result;
										output("%sFailed to write line #%lld of bytes.\n",M_ERROR_PREFIX,(result>0?result:-result));
									}
								}
							}
							lineListelement=lineListelement->_next;
						}
					}
				}else
				if(writeValue->type==VT_ARRAY){
					result=0; // assume all lines will be written
					Mvalue** values=writeValue->value._array->values;
					Mvalue* valueToWrite=values[0];
					long long numberOfWrites=writeValue->value._array->numberOfElements;
					while(numberOfWrites>0){
						if(result>=0)result++;else result--;
						if(valueToWrite->type==VT_TEXT){
							if(fWriteChars(_file,valueToWrite->value._text->_c,false)!=0)if(result>0)result=-result;
						}else
						if(valueToWrite->type==VT_BYTES){
							if(fWriteBytesToFile(_file,valueToWrite->value._string,false)!=0)if(result>0)result=-result;
						}
						++valueToWrite;
						--numberOfWrites;
					}
				}
				*/
			}else
			if(fileValue!=NULL)
				outputError("No file or file name specified to write to");
			else
				outputError("No file to write to");
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
			//else if(opened&&_file->_f!=NULL&&fClosed(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),true)!=M_TRUE)output("Failed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
		}
	}else
		outputError("No or invalid text to write");
	return _getIntegerValue(result);
}
/**
 * @brief write \p lineToWriteValue to file \p fileValue as a text line (by writing FILE_EOLN at the end)
 * 
 * @param fileValue the file to write to
 * @param lineToWriteValue the text to write
 * @return Mvalue* M_TRUE on success, M_FALSE on failure and M_LL_INVALID when \p fileValue is undefined or not a file or \p lineToWriteValue is undefined or not a text
 */
Mvalue* Mfwriteline(Mvalue* fileValue,Mvalue* writeValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(writeValue!=NULL){
		Mfile* _file=(fileValue!=NULL?(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL)):NULL);
		if(_file!=NULL){
			bool opened=false;
			if(NULL==_file->_f){
				if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"a+",false,false)==M_TRUE)
					opened=true;
				else
					output("%sFailed to open file '%s' to append to.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			if(_file->_f!=NULL){
				result=fWriteValue(_file->_f,writeValue,fIsOpenedInBinaryMode(_file)==M_TRUE);
				/* replacing:
				// MDH@24JUN2024: using the same code as Mfwrite() uses, and simply appending EOLN if writing succeeds
				if(fIsOpenedInBinaryMode(_file)!=M_TRUE){ // opened as text file, so we should write the text representation
					Mstring* _writeValueText=owned_string(_getValueText(writeValue,false,true),owner);
					if(_writeValueText!=NULL){
						result=fWriteString(_file->_f,_writeValueText);
						FREE_STRING(_writeValueText,owner);
					}else
						output("%sFailed to create the text to write to file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
				}else
					result=fWriteValueToBinaryFile(_file->_f,writeValue);
				*/
				// if written successfully write the EOLN
				if(result==0)result=fWriteChars(_file->_f,FILE_EOLN);
				if(opened)closeFile(_file,true);
				/* replacing:
				// MDH@29MAY2024: prefix a newline when lines were written before
				if(writeValue->type==VT_TEXT){
					result=fWriteCharsToFile(_file,writeValue->value._text->_c,true);
				}else
				if(writeValue->type==VT_BYTES){
					result=fWriteBytesToFile(_file,writeValue->value._string,true);
				}else{
					Mstring* _writeValueText=owned_string(_getValueText(writeValue,false,true),owner);
					if(_writeValueText!=NULL){
						result=fWriteString(_file->_f,_writeValueText);
						FREE_STRING(_writeValueText,owner);
					}
				}
				*/
			}else
				outputError("Can't write (a line) to an unopened file");
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
			///////else if(opened&&_file->_f!=NULL&&fClosed(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),true)!=M_TRUE)output("Failed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
		}else
		if(fileValue!=NULL)
			outputError("No file or file name specified to write a line to");
		else
			outputError("No file specified to write to");
	}else
		outputError("No line specified to write");
	return _getIntegerValue(result);
}

/**
 * @brief writes an end-of-line to \p fileValue 
 * 
 * @param fileValue the file to write an end-of-line to
 * @return Mvalue* 0 on success, >0 on failure, <0 when invalid input
 */
Mvalue* Mfnewline(Mvalue const * const fileValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	Mfile* _file=(fileValue!=NULL?(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL)):NULL);
	if(_file!=NULL){
		bool opened=false;
		if(NULL==_file->_f){
			if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"a+",false,false)==M_TRUE)
				opened=true;
			else
				output("%sFailed to open file '%s' to append an end-of-line to.\n",M_ERROR_PREFIX,string(_file->_name));
		}
		if(_file->_f!=NULL){
			result=fWriteChars(_file->_f,FILE_EOLN);
			if(opened)closeFile(_file,true);
		}
	}else
		outputError("No file to write an end-of-line to");
	return _getIntegerValue(result);
}

/**
 * @brief writes the lines in \p linesToWriteValue to the file \p fileValue
 * 
 * @param fileValue 
 * @param linesToWriteValue 
 * @return Mvalue* M_TRUE when all lines were written successfully, M_FALSE otherwise
 */
Mvalue* Mfwritelines(Mvalue const * const fileValue,Mvalue const * const linesToWriteValue){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	if(linesToWriteValue!=NULL){
		Mfile* _file=(fileValue!=NULL?(fileValue->type==VT_FILE?fileValue->value._file:(fileValue->type==VT_TEXT?owned_file(_getFile(fileValue->value._text->_c),owner):NULL)):NULL);
		if(_file!=NULL){
			bool opened=false;
			if(NULL==_file->_f){
				if(fOpened(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),"a+",false,false)==M_TRUE)
					opened=true;
				else
					output("%sFailed to open file '%s' to append to.\n",M_ERROR_PREFIX,string(_file->_name));
			}
			if(_file->_f!=NULL){
				bool openedInBinaryMode=(fIsOpenedInBinaryMode(_file)==M_TRUE);
				if(linesToWriteValue->type==VT_LIST){
					Mlist* linesToWrite=linesToWriteValue->value._list;
					result=(linesToWrite!=NULL?linesToWrite->numberOfElements:-1);
					if(result>0){
						output("Number of lines to write: %lld.\n",result);
						Mlistelement* lineListelement=linesToWrite->_first;
						Mvalue* lineListelementValue;
						while(lineListelement!=NULL){
							lineListelementValue=lineListelement->_value;
							lineListelement=lineListelement->_next;
							if(lineListelementValue!=NULL){
								if(fWriteValue(_file->_f,lineListelementValue,openedInBinaryMode)!=0){
									output(M_ERROR_PREFIX);outputValue("Failed to write '",lineListelementValue,"'.\n");
									break;
								}
								if(lineListelement!=NULL){
									if(fWriteChars(_file->_f,FILE_EOLN)!=0){
										outputError("Failed to write end-of-line");
										break;
									}
								}
							}
							// one less line to write
							result--;
							if(result==0){
								if(lineListelement!=NULL)output("%sNumber of elements in list to write to file '%s' invalid!\n",M_BUG_PREFIX,string(_file->_name));
								break;
							}
						}
					}else
					if(result<0)
						outputError("No list to write");
				}else
				if(linesToWriteValue->type==VT_ARRAY){
					Marray* arrayToWrite=linesToWriteValue->value._array;
					Mvalue** values=(arrayToWrite!=NULL?arrayToWrite->values:NULL);
					result=(values!=NULL?arrayToWrite->numberOfElements:-1);
					if(result>0){
						output("Number of lines to write: %lld.\n",result);
						while(result>0){
							//////if(result>=0)result++;else result--;
							Mvalue* valueToWrite=*values;
							++values;
							if(valueToWrite!=NULL){
								if(fWriteValue(_file->_f,valueToWrite,openedInBinaryMode)!=0){
									output(M_ERROR_PREFIX);outputValue("Failed to write '",valueToWrite,"'.\n");
									break;
								}
								// always write an end-of-line unless this is the last line to write in a file opened here
								if(result>1){
									if(fWriteChars(_file->_f,FILE_EOLN)!=0){
										outputError("Failed to write a newline");
										break;
									}
								}
								/* replacing:
								if(valueToWrite->type==VT_TEXT){
									result=fWriteCharsToFile(_file,valueToWrite->value._text->_c,true);
								}else
								if(valueToWrite->type==VT_BYTES){
									result=fWriteBytesToFile(_file,valueToWrite->value._string,true);
								}else{
									Mstring* _valueToWriteText=owned_string(_getValueText(valueToWrite,false,true),owner);
									if(_valueToWriteText!=NULL){
										result+=fWriteString(_file->_f,_valueToWriteText);
										FREE_STRING(_valueToWriteText,owner);
									}
								}
								*/
							}
							result--;
							/* replacing:
							if(Mfwriteline(_file,valueToWrite)!=M_TRUE){result=numberOfLinesToWrite;break;} // delegate to Mfwriteline() to write the single value text as a line (followed by an end-of-line)
							*/
						}
					}else
					if(result<0)
						output("%sNo array to write to file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
				}else{
					output("%sNo array of list specified to write to '%s'. Will write a single line.\n",M_WARNING_PREFIX,string(_file->_name));
					result=(fWriteValue(_file->_f,linesToWriteValue,openedInBinaryMode)==0&&fWriteChars(_file->_f,FILE_EOLN)==0?0:1);
				}
				if(opened)closeFile(_file,true);
			}
			if(fileValue->type!=VT_FILE)FREE_FILE(_file,owner);
			//else if(opened&&_file->_f!=NULL&&fClosed(_file,(fileValue->type==VT_FILE?getValueDataOwner():owner),true)!=M_TRUE)output("Failed to close file '%s'.\n",M_ERROR_PREFIX,string(_file->_name));
		}else
		if(fileValue!=NULL)
			outputError("No file or file name specified to write lines to");
		else
			outputError("No file to write to specified");
	}else
		outputError("No file to write to");
	return _getIntegerValue(result);
}

// for persisting variables, we need to be able to load and save values
// the values will be written as text but in such a way that they can be read ('loaded') without loss of precision
/**
 * @brief saves \p writeValue to the M file hosted in \p file_value
 * 
 * @param file_value 
 * @param write_value 
 * @return Mvalue* the number of bytes written
 */
Mvalue* Mfsave(Mvalue* file_value,Mvalue* write_value){Mallocationowner owner=getOwner(__LINE__);
	long long result=M_LL_INVALID;
	Mfile* _file=(file_value!=NULL&&file_value->type==VT_FILE?file_value->value._file:NULL);
	if(_file!=NULL){ // something to write to
		// delegate as much as possible to _getValueText()
	}
	return _getIntegerValue(result); 
}
/**
 * @brief loads a M value from the M file hosted by \p file_value
 * 
 * @param file_value 
 * @return Mvalue* the M value loaded
 */
Mvalue* Mfload(Mvalue* file_value){
	Mvalue* _result=NULL;
	Mfile* _file=(file_value!=NULL&&file_value->type==VT_FILE?file_value->value._file:NULL);
	if(_file!=NULL){ // something to read from which should currently be open for reading text from
	}	
	return _result;
}

/**
 * @brief returns a new M value hosting a list of file names in \p file_value
 * @details \p file_value is supposed to host a directory name
 * @param file_value 
 * @return Mvalue* a new M value hosting a list of file names in \p file_value
 */
Mvalue* Mfiles(Mvalue* file_value){Mallocationowner owner=getOwner(__LINE__);
	// let's allow either a text or a file, in any case we need a foldername
	if(file_value!=NULL){
		char* directoryname=NULL;
		if(file_value->type==VT_FILE){
			if(file_value->value._file!=NULL)
				if(file_value->value._file->_name!=NULL)
					directoryname=string(file_value->value._file->_name);
		}else
		if(file_value->type==VT_TEXT)directoryname=file_value->value._text->_c;
		if(directoryname!=NULL&&strlen(directoryname)){
			DIR* dr=opendir(directoryname);
			if(dr!=NULL){
				Mlist* files_list=owned_list(_getListOfType(VT_TEXT),owner);
				struct dirent *en;
				while((en=readdir(dr))!=NULL){
					Mstring* _filename=owned_string(_getString("'"),owner);
					if(_filename!=NULL){
						// let's NOT prepend the directory name!!!! string_append(_filename,directoryname);
						if(string_append(_filename,en->d_name)!=NULL){
							if(appendedToList(files_list,owner,_getTextValue(string(_filename)),M_LL_INVALID)<=0)
								outputError("Failed to append the name of a list");
						}else
							outputError("Failed to construct the name of a file");
						FREE_STRING(_filename,owner);
					}
				}
				closedir(dr); //close all directory
				return _getValueOfList(disowned_list(files_list,owner));
			}
		}
	}
	/* replacing:
	Mfile* _file=(file_value?file_value->value._file:NULL);
	if(_file&&_file->_name&&_file->_stat&&S_ISDIR(_file->_stat->st_mode)){ // the file is a directory
		char* directoryname=string(_file->_name);
		DIR* dr=(directoryname?opendir(directoryname):NULL);
		if(dr){
			Mlist* files_list=owned_list(_getListOfType(VT_TEXT),owner);
			struct dirent *en;
			while((en=readdir(dr))!=NULL){
				Mstring* _filename=owned_string(_getString("'"),owner);
				if(_filename){
					// let's NOT prepend the directory name!!!! string_append(_filename,directoryname);
					string_append(_filename,en->d_name);
					appendedToList(files_list,owner,_getTextValue(string(_filename)),M_LL_INVALID);
					FREE_STRING(_filename,owner);
				}
			}
			closedir(dr); //close all directory
			return _getValueOfList(disowned_list(files_list,owner));
		}
   }*/
   return NULL;
}
/**
 * @brief returns a new M value wrapping M time \p _time
 * 
 * @param _time 
 * @return Mvalue* a new M value wrapping M time \p _time
 */
Mvalue* _getValueOfTime(Mtime* _time){
	if(NULL==_time)return NULL;
	bool disowned_time=Misdisowned(_time);
	Mvalue* _value=__value("time");
	if(NULL==_value){
		if(disowned_time)free_time(_time);
		return NULL;
	}
	_value->type=VT_TIME;
	// MDH@12JUN2020: TODO supposedly this is a bit of a problem actually taking over the ownership of an environment completely
	_value->value._time=(disowned_time?owned_time(_time,owner_value_data):_time);
	return _value;
}

/**
 * @brief return M_TRUE if \p valuetype is a numeric value type, M_FALSE otherwise
 * 
 * @param valuetype 
 * @return long long M_TRUE if \p valuetype is a numeric value type, M_FALSE otherwise
 */
long long isANumericValuetype(Mvaluetype valuetype){
	return(valuetype==VT_BIGINTEGER||valuetype==VT_DECIMAL||valuetype==VT_FLOAT||valuetype==VT_INTEGER||valuetype==VT_RATIONAL?M_TRUE:M_FALSE);
}/* VALIDATED */

/**
 * @brief returns M_TRUE if \p _value is numeric, M_FALSE or M_LL_INVALID otherwise
 * @details returns M_LL_INVALID iff \p _value is NULL
 * @param _value 
 * @return long long M_TRUE if \p _value is numeric, M_FALSE or M_LL_INVALID otherwise
 */
long long isNumeric(Mvalue* _value){
	return(_value!=NULL?isANumericValuetype(_value->type):M_LL_INVALID);
}/* VALIDATED */

/**
 * @brief returns a copy of M rational \p _rational
 * 
 * @param _rational 
 * @return Mrational* a copy of M rational \p _rational
 */
Mrational* _getRationalCopy(Mrational const * const _rational){Mallocationowner owner=getOwner(__LINE__);
	if(NULL==_rational)return NULL;
	// MDH@28MAR2023: the serious thing here is that _getRational ALWAYS creates a copy of the numerator and denominator passed in
	//                therefore what we did here is no longer necessary
	Mrational* _copyRational=owned_rational(_getRational(_rational->num,_rational->den,(_rational->delta!=NULL?_rational->delta->ld:M_LD_NAN),false),owner);
	/* replacing:
	Mbiginteger* _numeratorBiginteger=NULL;
	Mbiginteger* _denominatorBiginteger=NULL;
	if(_rational->num!=NULL){
		_numeratorBiginteger=owned_biginteger(_getBigintegerCopy(_rational->num),owner);
		if(NULL==_numeratorBiginteger)return NULL;
	}
	if(_rational->den!=NULL){
		_denominatorBiginteger=owned_biginteger(_getBigintegerCopy(_rational->den),owner);
		if(NULL==_denominatorBiginteger){FREE_BIGINTEGER(_numeratorBiginteger,owner);return NULL;}
	}
	Mrational* _copyRational=owned_rational(_getRational(_numeratorBiginteger,_denominatorBiginteger,(_rational->delta?_rational->delta->ld:M_LD_NAN),false),owner);
	//// replacing:
	//if(NULL==_numeratorBiginteger||(NULL==_denominatorBiginteger&&_rational->den!=NULL)){
	//	FREE_BIGINTEGER(_numeratorBiginteger,owner);
	//	FREE_BIGINTEGER(_denominatorBiginteger,owner);
	//	return NULL;
	//} // some error
	//// MDH@13JUN2019: if we can't get a rational, free the numerator and denominator
	//_copyRational=owned_rational(_getRational(_numeratorBiginteger,_denominatorBiginteger,(_rational->delta?_rational->delta->ld:M_LD_NAN),false),owner);
	//FREE_BIGINTEGER(_numeratorBiginteger,owner);
	//FREE_BIGINTEGER(_denominatorBiginteger,owner);
	//if(NULL==_copyRational)return NULL;
	*/
	_copyRational->normalized=_rational->normalized; // copy the rational flag
	return disowned_rational(_copyRational,owner);
}

// _getValueRational() returns a (new) rational from the value stored in _value
/**
 * @brief returns the wrapped M rational of \p value
 * 
 * @param value 
 * @return Mrational* the wrapped M rational of \p value
 */
Mrational* _getValueRational(Mvalue const * const value){Mallocationowner owner=getOwner(__LINE__);
	Mrational* _rational=NULL;
	if(value!=NULL){
		if(amVerboseDebugging())outputValue("Extracting the rational from '",value,"'.\n");
		// MDH@28MAR2023: _getValueBiginteger will return a new big integer if value does not wrap a big integer
		//                in which case we need to free that new big integer as _getRational does NOT wrap the numerator/denominator
		switch(value->type){
			case VT_INTEGER:
				{
					Mbiginteger* value_bi=owned_biginteger(_getValueBiginteger(value),owner);
					if(NULL==value_bi){outputError("Failed to convert an integer to a big integer.");return NULL;}
					_rational=owned_rational(_getRational(value_bi,NULL,M_LD_NAN,false),owner); // not to free what's wrapped in _value
					FREE_BIGINTEGER(value_bi,owner);
				}
				break;
			case VT_BIGINTEGER:
				_rational=owned_rational(_getRational(_getValueBiginteger(value),NULL,M_LD_NAN,false),owner); // not to free what's wrapped in _value
				break;
			case VT_DECIMAL:
				_rational=owned_rational(_getDecimalRational(value->value._decimal),owner);
				/* replacing (and augmenting in case of a repeating fractional part):
				{ // until we find a way to get the associated rational using the internal representation we stick to extracting the rational from the text representation of the decimal (which should be exact)
					char* _decimalText=mpd_to_sci(_value->value._decimal->mpd,0);
					if(_decimalText){_rational=_getDecimalTextRational(_decimalText,false);free(_decimalText);}else output("ERROR: Failed to obtain the text representation of a decimal.");
				}
				*/
				break;
			case VT_TEXT:
				_rational=owned_rational(_getDecimalTextRational(value->value._text->_c),owner);
				break;
			case VT_BYTES:
				// TODO convert all stored bytes to rationals, and store them in an array
				break;
			case VT_RATIONAL:
				_rational=owned_rational(_getRationalCopy(value->value._rational),owner); // NOTE return a copy NOT the original rational, only Mvalue things are immutable and the reference count is kept (and you should not use its contents elsewhere!!!)
				break;
			case VT_FLOAT:
				_rational=owned_rational(_getLongDoubleRational(value->value._float->ld,250),owner); // TODO how many iterations at most???
				break;
			case VT_LIST:
				if(value->value._list->numberOfElements>1){
					// MDH@28MAR2023: the following will also be problematic we do not release the big integers created
					//                NOT allowing a numerator or denominator that cannot be converted to a big integer
					Mvalue* numeratorValue=value->value._list->_first->_value;
					Mvalue* denominatorValue=value->value._list->_first->_next->_value;
					Mbiginteger* bi_num=NULL;
					if(numeratorValue!=NULL){
						bi_num=_getValueBiginteger(numeratorValue);
						if(bi_num==NULL){outputError("Failed to convert the first listelement into a big integer");return NULL;}
					}
					Mbiginteger* bi_den=NULL;
					if(denominatorValue!=NULL){
						bi_den=_getValueBiginteger(denominatorValue);
						if(bi_den==NULL){
							// TODO whatever _getValueBiginteger returned hasn't be owned here, so call free_biginteger not FREE_BIGINTEGER
							if(bi_num!=NULL&&numeratorValue!=NULL&&numeratorValue->type!=VT_BIGINTEGER)free_biginteger(bi_num);
							outputError("Failed to convert the second list element to a big integer");
							return NULL;
						}
					}
					// either the numerator or the denominator needs to be non NULL (TODO why can't both be NULL???)
					if(bi_num!=NULL||bi_den!=NULL)
						_rational=owned_rational(_getRational(bi_num,bi_den,
						(value->value._list->numberOfElements>2?getValueLongDouble(value->value._list->_first->_next->_next->_value):M_LD_NAN),
						true),owner);
					if(bi_num!=NULL&&numeratorValue!=NULL&&numeratorValue->type!=VT_BIGINTEGER)free_biginteger(bi_num);
					if(bi_den!=NULL&&denominatorValue!=NULL&&denominatorValue->type!=VT_BIGINTEGER)free_biginteger(bi_den);
				}
				break;
			default:break;
		}
	}
	return disowned_rational(_rational,owner);
}

// MDH@11AUG2019: why wasn't this here before???
/**
 * @brief returns the M rational wrapped in \p value or a new M rational converted from whatever \p value wraps
 * @details _getValueRational will always return a new M rational (even if \p value contains a rational itself)
 * @param value 
 * @return Mrational* the M rational wrapped in \p value or a new M rational converted from whatever \p value wraps
 */
Mrational* getValueRational(Mvalue const * const value){//Mallocationowner owner=getOwner(__LINE__);
	if(value!=NULL&&value->type==VT_RATIONAL)return value->value._rational;
	return _getValueRational(value);
}

// MDH@20AUG2024: messages M functions
Mvalue* Mmessages(Mvalue const * const messageTypeValue){Mallocationowner owner=getOwner(__LINE__);
	Messages* messages=NULL;
	if(messageTypeValue!=NULL){
		if(messageTypeValue->type==VT_TEXT){
			messages=_getMessagesOfType(messageTypeValue->value._text->_c);
		}
		outputError("Invalid message type; will return all messages");
		messages=_getMessages();
	}else
		messages=_getMessages();
	if(messages!=NULL){
		///output("Number of messages: %zu.\n",messages->count);
		Marray* messagesArray=owned_array(_getArray("Mmessages",messages->count,NULL),owner);
		if(messagesArray!=NULL){
			size_t messageIndex=messages->count;
			while(messageIndex>0){
				Message* message=messages->messages[--messageIndex];
				if(NULL==message)continue;
				Mstring* msgText=owned_string(_getString("'"),owner);
				if(msgText==NULL){outputError("Failed to create text to store message in");continue;}
				if(message->id!=NULL&&strlen(message->id)){
					string_append_char(msgText,'(');
					string_append(msgText,message->id);
					string_append(msgText,") ");
				}
				if(messages->types[messageIndex]!=NULL){
					string_append(msgText,messages->types[messageIndex]);
					///////string_append(msgText,": ");
				}
				string_append(msgText,message->msg);
				assignValue(messagesArray->values+messageIndex,_getValueOfText(_getText(string(msgText))));
				FREE_STRING(msgText,owner);
			}
		}
		// essential to free the (unmanaged) messages
		free_messages(messages);
		if(messagesArray!=NULL)
			return _getValueOfArray(disowned_array(messagesArray,owner));
	}else
		output("No messages!\n");
	return NULL;
}
/**
 * @brief removes all messages of type \p messageTypeValue
 * 
 * @param messageTypeValue 
 * @return Mvalue* the number of unremoved messages, or M_LL_INVALID when the type is not registered
 */
Mvalue* Mremovemessages(Mvalue const * const messageTypeValue){
	long long result=M_LL_INVALID;
	if(messageTypeValue!=NULL){
		if(messageTypeValue->type==VT_TEXT){
			result=removeMessagesOfType(messageTypeValue->value._text->_c);
			if(result<0)result=M_LL_INVALID;
		}
	}else // removing all messages (not the types)
		result=removeMessagesOfType(NULL);
	return _getIntegerValue(result);
}
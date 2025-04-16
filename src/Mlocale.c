#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "Mlocale.h"

extern unsigned long long M_MODULE_DEBUGGING;

static Mallocationowner getOwner(uint16_t id){return(Mallocationowner){MI_LOCALE,id};}
/*
#define INT_STR_SIZE (CHAR_BIT*sizeof(int)*3/10 + 2)
#define INT_SEP_STR_SIZE (INT_STR_SIZE * 3/2 + 1)
#define INT_SEP(x) int_sep((char[INT_SEP_STR_SIZE]) { "" }, INT_SEP_STR_SIZE, x)
*/

// keep all location settings in a single map
/**
 * @brief the global M map containing the locale settings
 * 
 */
static Mmap* _localesettingsMap=NULL;Mallocationowner owner_localesettingsmap=(Mallocationowner){MI_LOCALE,__LINE__,1};

// call updateLocaleSettingsMap whenever some of the current locale settings changed (typically when the locale was changed)
/**
 * @brief updates the global locale settings map
 * 
 * @return true 
 * @return false 
 */
bool updateLocalesettingsMap(){
	char* _locale=setlocale(LC_ALL,NULL);
	if(_localesettingsMap!=NULL){
		if(_locale!=NULL)q2output("Active locale: %s.\n",_locale);else q2outputError("No active locale!");
		if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"",_getValueOfText(_getSingleQuotedText(_locale)))>0){
			//D q2output("Locale name added to locale settings map.\n"); // DEBUGGING
			// now we should append the locale settings
			struct lconv* localeSettings=localeconv();
			if(localeSettings!=NULL){
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"decimal_point",_getValueOfText(_getSingleQuotedText(localeSettings->decimal_point)))<=0)
					q2outputError("Failed to store the decimal_point locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"thousands_sep",_getValueOfText(_getSingleQuotedText(localeSettings->thousands_sep)))<=0)
					q2outputError("Failed to store the thousands_sep locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"grouping",_getValueOfText(_getSingleQuotedText(localeSettings->grouping)))<=0)
					q2outputError("Failed to store the grouping locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"int_curr_symbol",_getValueOfText(_getSingleQuotedText(localeSettings->int_curr_symbol)))<=0)
					q2outputError("Failed to store the int_curr_symbol locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"currency_symbol",_getValueOfText(_getSingleQuotedText(localeSettings->currency_symbol)))<=0)
					q2outputError("Failed to store the currency_symbol locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_decimal_point",_getValueOfText(_getSingleQuotedText(localeSettings->mon_decimal_point)))<=0)
					q2outputError("Failed to store the mon_decimal_point locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_thousands_sep",_getValueOfText(_getSingleQuotedText(localeSettings->mon_thousands_sep)))<=0)
					q2outputError("Failed to store the mon_thousands_sep locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_grouping",_getValueOfText(_getSingleQuotedText(localeSettings->mon_grouping)))<=0)
					q2outputError("Failed to store the mon_grouping locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"positive_sign",_getValueOfText(_getSingleQuotedText(localeSettings->positive_sign)))<=0)
					q2outputError("Failed to store the positive_sign locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"negative_sign",_getValueOfText(_getSingleQuotedText(localeSettings->negative_sign)))<=0)
					q2outputError("Failed to store the decimal point locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"int_frac_digits",_getValueOfText(_getSingleQuotedCharText(localeSettings->int_frac_digits)))<=0)
					q2outputError("Failed to store the int_frac_digits locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"frac_digits",_getValueOfText(_getSingleQuotedCharText(localeSettings->frac_digits)))<=0)
					q2outputError("Failed to store the frac_digits locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_cs_precedes",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_cs_precedes)))<=0)
					q2outputError("Failed to store the p_cs_precedes locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_sep_by_space",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_sep_by_space)))<=0)
					q2outputError("Failed to store the p_sep_by_space locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_cs_precedes",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_cs_precedes)))<=0)
					q2outputError("Failed to store the n_cs_precedes locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_sep_by_space",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_sep_by_space)))<=0)
					q2outputError("Failed to store the n_sep_by_space locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_sign_posn",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_sign_posn)))<=0)
					q2outputError("Failed to store the p_sign_posn locale setting");
				if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_sign_posn",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_sign_posn)))<=0)
					q2outputError("Failed to store the n_sign_posn locale setting");
				return true;
			}
			q2outputError("No locale settings found!");
		}else
			q2outputError("Failed to add the locale name to the locale settings map!");
	}
	return false;
}

// are we supposed to update the locale every time getLocaleValue() is called???????
// I suppose we could update the map every time???? i.e. the map is mutable but localeValue is not (i.e. you can't change the map it contains)
/**
 * @brief returns the global locale settings map
 * @details always updates the global local settings map
 * @return Mmap* the global locale settings map
 */
Mmap* getLocalesettingsMap(){///////////Mallocationowner owner=getOwner(__LINE__);
	// if the current locale value is not bound somehow, we create a new one, essentially allowing external party to get a new locale value created
	if(NULL==_localesettingsMap)
		_localesettingsMap=owned_map(__map("getLocaleSettingsMap()"),owner_localesettingsmap);
	if(_localesettingsMap!=NULL){
		//D q2output("Locale settings map created!\n"); // DEBUGGING
		if(!updateLocalesettingsMap())
			q2outputError("Failed to register the current locale settings");
		//D else q2output("Locale settings map updated!\n"); // DEBUGGING
	}else
		q2outputError("Failed to create the map for storing the locale settings");
	// NOTE not returning the map disowned, so this module will keep ownership of the map
	return _localesettingsMap; //////disowned_map(_localesettingsMap,owner_localesettingsmap); 
}
/*
static char *int_sep(char *s, size_t sz, int x){
  struct lconv *locale_ptr = localeconv();
  const char *grouping = locale_ptr->grouping;
  char sep = locale_ptr->thousands_sep[0];
  // MDH@07DEC2020: somebody forgot to take into account that the thousands separator could be '\0' so we have to append sep & to the text at the start of the do while loop
  if (sz > 0) {
    int x0 = x;
    char *ptr = s + sz;
    *--ptr = '\0';
    char count = 0;
    do {
      if (sep && count >= grouping[0]) {
        *--ptr = sep;
        if (grouping[1]) grouping++;
        count = 0;
      }
      count++;
      //printf("%d %d <%s> %p\n", count, n, locale_ptr->grouping, (void*)locale_ptr);
      *--ptr = (char) (abs(x % 10) + '0');
    } while (x /= 10);
    if (x0 < 0) {
      *--ptr = '-';
    }
    memmove(s, ptr, (size_t) (&s[sz] - ptr));
  }
  return s;
}
*/
/**
 * @brief returns the text representation of long integer \p ll in the current locale
 * @param s to store the text representation
 * @param sz the maximum allowed number of characters in s
 * @param x
 * @return the text representation of long integer \p x in the current locale
*/
static char *ll_sep(char *s, size_t sz, long long ll){
	struct lconv *locale_ptr=localeconv();
	const char *grouping=(locale_ptr!=NULL?locale_ptr->grouping:NULL);
	char sep=(grouping!=NULL?locale_ptr->thousands_sep[0]:'\0'); // ascertain that sep is '\0' when grouping is NULL, so grouping will NOT be used
	// MDH@07DEC2020: somebody forgot to take into account that the thousands separator could be '\0' so we have to append sep & to the text at the start of the do while loop
	if(sz>0){
		long long x0=ll;
		char *ptr=s+sz;
		*--ptr='\0';
		char count=0;
		do{
			if(sep&&count>=grouping[0]){
				*--ptr=sep;
				if(grouping[1])grouping++;
				count=0;
			}
			count++;
			//printf("%d %d <%s> %p\n", count, n, locale_ptr->grouping, (void*)locale_ptr);
			*--ptr=(char)(llabs(ll%10)+'0');
		}while(ll/=10);
		if(x0<0)*--ptr='-';
		memmove(s,ptr,(size_t)(&s[sz]-ptr));
	}
	return s;
}

#define LL_STR_SIZE (CHAR_BIT*sizeof(long long)*3/10 + 4)
#define LL_SEP_STR_SIZE (LL_STR_SIZE * 3/2 + 1)
#define LL_SEP(ll) ll_sep((char[LL_SEP_STR_SIZE]){ "" },LL_SEP_STR_SIZE,ll)
/**
 * @brief outputs long integer \p ll in the current locale
 * 
 * @param ll 
 * @return size_t the number of characters written
 */
size_t outputLongLongLocale(const long long ll){return output("%s",LL_SEP(ll));}
/**
 * @brief outputs M integer \p integer in the current locale
 * 
 * @param integer 
 * @return size_t the number of characters written
 */
size_t outputIntegerLocale(Minteger const * const integer){
	return(integer!=NULL?outputLongLongLocale(integer->ll):0);
}

// getIntegerTextLocale() returns the text of the integer provided formatted in using the current locale thousand separator
/**
 * @brief returns the text representation of the integer represented by text \p integerText using the current locale thousand separator
 * 
 * @param integerText 
 * @return Mstring* the text representation of the integer represented by text \p integerText using the current locale thousand separator
 */
static Mstring* _getIntegerTextLocale(char const * const integerText){Mallocationowner owner=getOwner(__LINE__);
	struct lconv *locale_ptr=localeconv();
	char sep=(locale_ptr!=NULL?locale_ptr->thousands_sep[0]:'\0');
	if(!sep){/*D output("No separator!"); D*/return NULL;} // this is easiest, so that integerText will simply be output instead
	// initialize _integerTextString with integerText
	Mstring* _integerTextString=owned_string(_getString(integerText),owner);
	if(NULL==_integerTextString)return NULL;
	const char *grouping=locale_ptr->grouping;
	if(NULL==grouping)return NULL; // NOTE should actually not happen though
	// take the sign into account
	size_t groupindex=0,sepinsertpos=string_length(_integerTextString),sign=(string_char(_integerTextString,0)=='-'?1:0);
	uint8_t groupsize=3,group=grouping[groupindex]; // assume the group size is 3
	while(group!=CHAR_MAX){
		// if group is not '\0' which means keep using the current groupsize, use group as groupsize and get the next one (which could be zero)
		if(group){
			groupsize=group;
			group=grouping[++groupindex];
		}
		if(groupsize<=sepinsertpos-sign)break;
		sepinsertpos-=groupsize;
		if(NULL==string_insert_char(_integerTextString,sepinsertpos,sep))break;
		output("->'%s'",string(_integerTextString));
	}
	return disowned_string(_integerTextString,owner);
}
/**
 * @brief outputs big integer \p biginteger
 * 
 * @param biginteger 
 * @return size_t the number of characters written
 */
size_t outputBigintegerLocale(Mbiginteger const * const biginteger){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	Mstring* _bigintegerText=owned_string(_getBigintegerText(biginteger),owner);
	if(_bigintegerText!=NULL){
		//D output("Big integer text: '%s'.\n",string(_bigintegerText));
		Mstring* _bigintegerTextLocale=owned_string(_getIntegerTextLocale(string(_bigintegerText)),owner);
		if(_bigintegerTextLocale!=NULL){ // we've got it
			written=output("%s",string(_bigintegerTextLocale));
			output("Freeing big integer text locale '%s'.\n",string(_bigintegerTextLocale)); // DEBUGGING
			FREE_STRING(_bigintegerTextLocale,owner);
		}else
			written=output("%s",string(_bigintegerText));
		//D output("Freeing big integer text!\n"); // DEBUGGING
		FREE_STRING(_bigintegerText,owner);
	}
	return written;
}

/** TODO
 * @brief returns the float text representation in the current locale
 * 
 * @param floatText 
 * @return Mstring* 
 */
static Mstring* _getFloatTextLocale(char const * const floatText){
	return NULL;
}
/**
 * @brief outputs M float \p _float calling _getFloatTextLocale() to get the text representation
 * 
 * @param _float 
 * @return size_t the number of characters output
 */
size_t outputFloatLocale(Mfloat const * const _float){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	Mstring* _floatText=owned_string(_getFloatText(_float),owner);
	if(_floatText!=NULL){
		Mstring* _floatTextLocale=owned_string(_getFloatTextLocale(string(_floatText)),owner);
		if(_floatTextLocale!=NULL){ // we've got it
			written=output("%s",string(_floatTextLocale));
			FREE_STRING(_floatTextLocale,owner);
		}else
			written=output("%s",string(_floatText));
		FREE_STRING(_floatText,owner);
	}
	return written;
}
/**
 * @brief outputs M decimal \p _decimal calling getFloatTextLocale() to get the text representation
 * 
 * @param _decimal 
 * @param fixedpoint 
 * @return size_t the number of characters written
 */
size_t outputDecimalLocale(Mdecimal const * const _decimal,bool fixedpoint){Mallocationowner owner=getOwner(__LINE__);
	size_t written=0;
	Mstring* _decimalText=owned_string(_getDecimalText(_decimal,fixedpoint),owner);
	if(_decimalText!=NULL){
		Mstring* _decimalTextLocale=owned_string(_getFloatTextLocale(string(_decimalText)),owner);
		if(_decimalTextLocale!=NULL){ // we've got it
			written=output("%s",string(_decimalTextLocale));
			FREE_STRING(_decimalTextLocale,owner);
		}else
			written=output("%s",string(_decimalText));
		FREE_STRING(_decimalText,owner);
	}
	return written;
}
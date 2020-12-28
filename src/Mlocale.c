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
static Mmap* _localesettingsMap=NULL;Mallocationowner owner_localesettingsmap=(Mallocationowner){MI_LOCALE,__LINE__,1};

// call updateLocaleSettingsMap whenever some of the current locale settings changed (typically when the locale was changed)
bool updateLocalesettingsMap(){
  char* _locale=setlocale(LC_ALL,NULL);
  if(_localesettingsMap){
    if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"",_getValueOfText(_getSingleQuotedText(_locale)))>0){
      // now we should append the locale settings
      struct lconv* localeSettings=localeconv();
      if(localeSettings){
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"decimal_point",_getValueOfText(_getSingleQuotedText(localeSettings->decimal_point)))<=0)
          outputError("Failed to store the decimal_point locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"thousands_sep",_getValueOfText(_getSingleQuotedText(localeSettings->thousands_sep)))<=0)
          outputError("Failed to store the thousands_sep locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"grouping",_getValueOfText(_getSingleQuotedText(localeSettings->grouping)))<=0)
          outputError("Failed to store the grouping locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"int_curr_symbol",_getValueOfText(_getSingleQuotedText(localeSettings->int_curr_symbol)))<=0)
          outputError("Failed to store the int_curr_symbol locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"currency_symbol",_getValueOfText(_getSingleQuotedText(localeSettings->currency_symbol)))<=0)
          outputError("Failed to store the currency_symbol locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_decimal_point",_getValueOfText(_getSingleQuotedText(localeSettings->mon_decimal_point)))<=0)
          outputError("Failed to store the mon_decimal_point locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_thousands_sep",_getValueOfText(_getSingleQuotedText(localeSettings->mon_thousands_sep)))<=0)
          outputError("Failed to store the mon_thousands_sep locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"mon_grouping",_getValueOfText(_getSingleQuotedText(localeSettings->mon_grouping)))<=0)
          outputError("Failed to store the mon_grouping locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"positive_sign",_getValueOfText(_getSingleQuotedText(localeSettings->positive_sign)))<=0)
          outputError("Failed to store the positive_sign locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"negative_sign",_getValueOfText(_getSingleQuotedText(localeSettings->negative_sign)))<=0)
          outputError("Failed to store the decimal point locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"int_frac_digits",_getValueOfText(_getSingleQuotedCharText(localeSettings->int_frac_digits)))<=0)
          outputError("Failed to store the int_frac_digits locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"frac_digits",_getValueOfText(_getSingleQuotedCharText(localeSettings->frac_digits)))<=0)
          outputError("Failed to store the frac_digits locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_cs_precedes",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_cs_precedes)))<=0)
          outputError("Failed to store the p_cs_precedes locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_sep_by_space",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_sep_by_space)))<=0)
          outputError("Failed to store the p_sep_by_space locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_cs_precedes",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_cs_precedes)))<=0)
          outputError("Failed to store the n_cs_precedes locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_sep_by_space",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_sep_by_space)))<=0)
          outputError("Failed to store the n_sep_by_space locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"p_sign_posn",_getValueOfText(_getSingleQuotedCharText(localeSettings->p_sign_posn)))<=0)
          outputError("Failed to store the p_sign_posn locale setting");
        if(appendedToMap(_localesettingsMap,owner_localesettingsmap,"n_sign_posn",_getValueOfText(_getSingleQuotedCharText(localeSettings->n_sign_posn)))<=0)
          outputError("Failed to store the n_sign_posn locale setting");
        return true;
      }
    }
  }
  return false;
}

// are we supposed to update the locale every time getLocaleValue() is called???????
// I suppose we could update the map every time???? i.e. the map is mutable but localeValue is not (i.e. you can't change the map it contains)
Mmap* getLocalesettingsMap(){Mallocationowner owner=getOwner(__LINE__);
  // if the current locale value is not bound somehow, we create a new one, essentially allowing external party to get a new locale value created
  if(!_localesettingsMap)
    _localesettingsMap=owned_map(__map("getLocaleSettingsMap()"),owner_localesettingsmap);
  if(_localesettingsMap){
      if(!updateLocalesettingsMap())
        outputError("Failed to register the current locale settings");
  }else
    outputError("Failed to create the map for storing the locale settings");
  return _localesettingsMap; // NOTE not returning the map disowned, so this module will keep ownership of the map
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
static char *ll_sep(char *s, size_t sz, long long x){
  struct lconv *locale_ptr = localeconv();
  const char *grouping = locale_ptr->grouping;
  char sep = locale_ptr->thousands_sep[0];
  // MDH@07DEC2020: somebody forgot to take into account that the thousands separator could be '\0' so we have to append sep & to the text at the start of the do while loop
  if (sz > 0) {
    long long x0 = x;
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
      *--ptr = (char) (llabs(x % 10) + '0');
    } while (x /= 10);
    if (x0 < 0) {
      *--ptr = '-';
    }
    memmove(s, ptr, (size_t) (&s[sz] - ptr));
  }
  return s;
}
#define LL_STR_SIZE (CHAR_BIT*sizeof(long long)*3/10 + 4)
#define LL_SEP_STR_SIZE (LL_STR_SIZE * 3/2 + 1)
#define LL_SEP(x) ll_sep((char[LL_SEP_STR_SIZE]){ "" },LL_SEP_STR_SIZE,x)
size_t outputLongLongLocale(long long ll){return output("%s",LL_SEP(ll));}
size_t outputIntegerLocale(Minteger* integer){
  return(integer?outputLongLongLocale(integer->ll):0);
}

// getIntegerTextLocale() returns the text of the integer provided formatted in using the current locale thousand separator
static Mstring* _getIntegerTextLocale(char* integerText){Mallocationowner owner=getOwner(__LINE__);
  struct lconv *locale_ptr=localeconv();
  char sep=(locale_ptr?locale_ptr->thousands_sep[0]:'\0');
  if(!sep)return NULL; // this is easiest, so that integerText will simply be output instead
  // initialize _integerTextString with integerText
  Mstring* _integerTextString=owned_string(_getString(integerText),owner);
  if(!_integerTextString)return NULL;
  const char *grouping=locale_ptr->grouping;
  if(!grouping)return NULL; // NOTE should actually not happen though
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
    if(!string_insert_char(_integerTextString,sepinsertpos,sep))break;
  }
  return disowned_string(_integerTextString,owner);
}
size_t outputBigintegerLocale(Mbiginteger* biginteger){Mallocationowner owner=getOwner(__LINE__);
  size_t written=0;
  Mstring* _bigintegerText=owned_string(_getBigintegerText(biginteger),owner);
  if(_bigintegerText){
    Mstring* _bigintegerTextLocale=owned_string(_getIntegerTextLocale(string(_bigintegerText)),owner);
    if(_bigintegerTextLocale){ // we've got it
      written=output("%s",string(_bigintegerTextLocale));
      FREE_STRING(_bigintegerTextLocale,owner);
    }else
      written=output("%s",string(_bigintegerText));
    FREE_STRING(_bigintegerText,owner);
  }
  return written;
}

static Mstring* _getFloatTextLocale(char* floatText){
  return NULL;
}
size_t outputFloatLocale(Mfloat* _float){Mallocationowner owner=getOwner(__LINE__);
  size_t written=0;
  Mstring* _floatText=owned_string(_getFloatText(_float),owner);
  if(_floatText){
    Mstring* _floatTextLocale=owned_string(_getFloatTextLocale(string(_floatText)),owner);
    if(_floatTextLocale){ // we've got it
      written=output("%s",string(_floatTextLocale));
      FREE_STRING(_floatTextLocale,owner);
    }else
      written=output("%s",string(_floatText));
    FREE_STRING(_floatText,owner);
  }
  return written;
}
size_t outputDecimalLocale(Mdecimal* _decimal,bool fixedpoint){Mallocationowner owner=getOwner(__LINE__);
  size_t written=0;
  Mstring* _decimalText=owned_string(_getDecimalText(_decimal,fixedpoint),owner);
  if(_decimalText){
    Mstring* _decimalTextLocale=owned_string(_getFloatTextLocale(string(_decimalText)),owner);
    if(_decimalTextLocale){ // we've got it
      written=output("%s",string(_decimalTextLocale));
      FREE_STRING(_decimalTextLocale,owner);
    }else
      written=output("%s",string(_decimalText));
    FREE_STRING(_decimalText,owner);
  }
  return written;
}
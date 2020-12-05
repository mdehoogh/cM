#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "Mlocale.h"

extern unsigned long long M_MODULE_DEBUGGING;
#define DEBUGGING (M_MODULE_DEBUGGING|MM_LOCALE)

char *int_sep(char *s, size_t sz, int x){
  struct lconv *locale_ptr = localeconv();
  const char *grouping = locale_ptr->grouping;
  char sep = locale_ptr->thousands_sep[0];

  if (sz > 0) {
    int x0 = x;
    char *ptr = s + sz;
    *--ptr = '\0';
    char count = 0;
    do {
      if (count >= grouping[0]) {
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

char *ll_sep(char *s, size_t sz, long long x){
  struct lconv *locale_ptr = localeconv();
  const char *grouping = locale_ptr->grouping;
  char sep = locale_ptr->thousands_sep[0];

  if (sz > 0) {
    long long x0 = x;
    char *ptr = s + sz;
    *--ptr = '\0';
    char count = 0;
    do {
      if (count >= grouping[0]) {
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
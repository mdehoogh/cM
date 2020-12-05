#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define INT_STR_SIZE (CHAR_BIT*sizeof(int)*3/10 + 2)
#define INT_SEP_STR_SIZE (INT_STR_SIZE * 3/2 + 1)
#define INT_SEP(x) int_sep((char[INT_SEP_STR_SIZE]) { "" }, INT_SEP_STR_SIZE, x)

char *int_sep(char *s, size_t sz, int x) {
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
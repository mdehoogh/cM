#include <stdlib.h>

#include "Mmemory.h"

#define INT_STR_SIZE (CHAR_BIT*sizeof(int)*3/10 + 2)
#define INT_SEP_STR_SIZE (INT_STR_SIZE * 3/2 + 1)
#define INT_SEP(x) int_sep((char[INT_SEP_STR_SIZE]) { "" }, INT_SEP_STR_SIZE, x)

char *int_sep(char *s, size_t sz, int x);

#define LL_STR_SIZE (CHAR_BIT*sizeof(long long)*3/10 + 4)
#define LL_SEP_STR_SIZE (LL_STR_SIZE * 3/2 + 1)
#define LL_SEP(x) ll_sep((char[LL_SEP_STR_SIZE]){ "" },LL_SEP_STR_SIZE,x)

char *ll_sep(char *s, size_t sz, long long x);

#!/bin/bash
rm M
/usr/bin/cc -ltommath mstring.c Msettings.c Mcolors.c Moutput.c Msession.c Mexpression.c Mmemory.c Mexecution.c M.c -o M -L . && ./M "$@"

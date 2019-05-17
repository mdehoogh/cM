#!/bin/bash
rm M
gcc mstring.c Msettings.c Mcolors.c Moutput.c Msession.c Mexpression.c Mmemory.c Mexecution.c M.c -o M && ./M "$@"

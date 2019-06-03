#!/bin/bash

# force running as sudo check
if [[ ! "$EUID" = 0 ]]; then
	echo "Please sudo run me!"
	exit 1
fi

# the actual stuff
# remove M so if we fail we'd know it
rm -f M

# if libtommath.a is not present, we should create it
if [[ ! -f ./libtommath.a && ! -d ./libtommath.a ]]; then
    echo "Creating tommath big integer library..."
    if [ -d libtommath ]; then
        # ascertain to hold tommath.h
        if [ ! -f tommath.h ]; then
		    cp libtommath/tommath.h .
		    if [ ! -f tommath.h ]; then
                echo "ERROR: Failed to copy 'tommath.h'."
		        exit 1
            else
                sudo chmod 644 tommath.h
            fi
	    fi
	    echo "Will attempt to create 'libtommath.a'..."
	    # can I run the make from here?????
	    cd libtommath
        # we want make to actually create the .o files now
        rm -f *.o
        rm -f *.a
	    sudo make
	    if [ -f libtommath.a ]; then
	        cd ..
	        cp libtommath/libtommath.a .
	        if [ -f libtommath.a ]; then
                # ascertain that we can at least read it
                sudo chmod 644 libtommath.a
            fi
        else
            echo "ERROR: Failed to copy libtommath.a into the cM directory!"
            exit 1
        fi
    else
        echo "ERROR: Cannot create libtommath.a archive - source directory libtommath is missing!"
        exit 1
    fi
fi

echo "Compiling..."
/usr/bin/cc  -L . -ltommath mstring.c Msettings.c Mcolors.c Moutput.c Msession.c Mexpression.c Mmemory.c Mexecution.c M.c -o M

# don't run M here, instead check whether it is there!!!
if [ -f M ]; then
	# ascertain to be able to run it
	sudo chmod 755 M
	echo "M has been created, run it with ./M!"
    echo "Command-line flags (specify behind a hyphen) include D (debug), s (verbose), A (assist)"
else
    echo "ERROR: Failed to create the M executable!"
fi

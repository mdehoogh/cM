#!/bin/bash

# MDH@21OCT2019: assuming that the M source files are in subfolder src/ now and the library files in lib/

# the actual stuff
# remove M so if we fail we'd know it
rm -f M

# if libmpdec.a is not present, we should create it
if [[ ! -f lib/libmpdec.a && ! -d lib/libmpdec.a ]]; then
#    # force running as sudo check
#    if [[ ! "$EUID" = 0 ]]; then
#        echo "Please sudo run me (in order to be able to create the decimal library)!"
#        exit 1
#    fi
    echo "Creating mpdecimal library..."
    if [ -d mpdecimal-2.4.2 ]; then
        # mpdecimal.h won't exist in libmpdec there until we run ./configure
        cd mpdecimal-2.4.2
        ./configure
        cd ..
        # ascertain to hold mpdecimal.h
        if [ ! -f src/mpdecimal.h ]; then
            cp mpdecimal-2.4.2/libmpdec/mpdecimal.h src/
            if [ ! -f src/mpdecimal.h ]; then
                echo "ERROR: Failed to copy 'mpdecimal.h' from the mpdecimal-2.4.2/libmpdec subdirectory to the src/ subdirectory."
                exit 1
            else
                chmod 644 src/mpdecimal.h
            fi
        fi
        echo "Will attempt to create 'libmpdec.a'..."
        cd mpdecimal-2.4.2
        echo "Ready for compile the mpdecimal static library..."
        # we want make to actually create the .o files now
        sudo make
        if [ -f libmpdec/libmpdec.a ]; then
            echo "Static library 'libmpdec.a' created successfully!"
            cd ..
            cp mpdecimal-2.4.2/libmpdec/libmpdec.a lib/
            if [ -f lib/libmpdec.a ]; then
                # ascertain that we can at least read it
                chmod 644 lib/libmpdec.a
            else
                echo "ERROR: Failed to copy 'libmpdec.a' from the mpdecimal-2.4.2/libmpdec subdirectory to the lib/ subdirectory."
                exit 1
            fi
        else
            echo "ERROR: Failed to create static library 'libmpdec.a'!"
            exit 1
        fi
    else
        echo "ERROR: Cannot create the libmpdec.a static library - source directory mpdecimal-2.4.2 is missing!"
        exit 1
    fi
else
    echo "NOTE: Decimal static library present!"
fi

# if libtommath.a is not present, we should create it
if [[ ! -f lib/libtommath.a && ! -d lib/libtommath.a ]]; then
#    # force running as sudo check
#    if [[ ! "$EUID" = 0 ]]; then
#        echo "Please sudo run me (in order to be able to create the big integer library)!"
#        exit 1
#    fi
    echo "Creating tommath big integer library..."
    if [ -d libtommath ]; then
        # ascertain to hold tommath.h
        if [ ! -f src/tommath.h ]; then
            echo "Will copy 'tommath.h' from the libtommath subdirectory..."
            cp libtommath/tommath.h src/
            if [ ! -f tommath.h ]; then
                echo "ERROR: Failed to copy 'tommath.h' to the src/ subdirectory."
                exit 1
            else
                chmod 644 tommath.h
            fi
        fi
        echo "Will attempt to create 'libtommath.a'..."
        # can I run the make from here?????
        cd libtommath
        # we want make to actually create the .o files now
        rm -f *.o
        rm -f *.a
        echo "About to create the libtommath static library..."
        sudo make
        if [ -f libtommath.a ]; then
            echo "Static library 'libtommath.a' created successfully!"
            cd ..
            cp libtommath/libtommath.a lib/
            if [ -f lib/libtommath.a ]; then
                # ascertain that we can at least read it
                chmod 644 lib/libtommath.a
            else
                echo "ERROR: Failed to copy static library 'libtommath.a' from the libtommath subdirectory to the lib subdirectory."
                exit 1
            fi
        else
            echo "ERROR: Failed to create static library 'libtommath.a' in the libtommath subdirectory!"
            exit 1
        fi
    else
        echo "ERROR: Cannot create libtommath.a archive - source directory libtommath is missing!"
        exit 1
    fi
else
    echo "NOTE: Big integer static library present!"
fi

echo ""
echo "Compiling M..."
cd src
cc -Wincompatible-pointer-types -Wdangling-else -Wincompatible-pointer-types-discards-qualifiers -L ../lib -ltommath -lmpdec Malloc.c Mstring.c Msettings.c Mcolors.c Moutput.c Msession.c Mexpression.c Mmemory.c Mexecution.c Mbiginteger.c Mrational.c Mdecimal.c Mvalue.c Mfunctions.c Menvironment.c M.c -o ../M 
cd ..
# don't run M here, instead check whether it is there!!!
if [ -f M ]; then
    # ascertain to be able to run it (when being sudo run)
    if [[ "$EUID" = 0 ]]; then
        chmod 755 M
        echo "M has been created, run it with ./M!"
    else
        echo "M has been created but you may not be able to run it with ./M if the next command (chmod) fails."
        chmod 755 M
    fi
#    echo "Command-line flags (specify behind a hyphen) include D (debug), s (verbose), A (assist)"
else
    echo "ERROR: Failed to create the M executable!"
fi

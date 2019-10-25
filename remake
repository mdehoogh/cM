#!/bin/bash
#./configure
rm -f src/M
make
if [[ -f src/M ]]; then
    echo
    echo "M built, cleaning up behind it..."
    rm -f src/*.o
    rm -f M
    mv src/M .
    echo
    if [[ -f M ]]; then
        echo "M built successfully, use ./M to run it."
    else
        echo "WARNING: M built successfully but moving it to the working directory failed. You have to move it yourself with: mv src/M ."
    fi
else
    echo "ERROR: Failed to build M."
fi
echo

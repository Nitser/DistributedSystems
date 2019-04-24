#!/bin/bash
PWD="$(pwd)"
PATH_TO_LIBRARY_DIR="$PWD/lib64"
PATH_TO_RUNTIME_LIB="$PATH_TO_LIBRARY_DIR/libruntime.so"

if [ ! "$LD_LIBRARY_PATH" == *_"$PATH_TO_LIBRARY_DIR"_* ]; then
    if [ -z "$LD_LIBRARY_PATH" ]; then
        export LD_LIBRARY_PATH="$PATH_TO_LIBRARY_DIR"
    else
        export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$PATH_TO_LIBRARY_DIR"
    fi
fi

LD_PRELOAD="$PATH_TO_RUNTIME_LIB"

clang --library-directory "$PATH_TO_LIBRARY_DIR" -std=c99 -Wall -pedantic *.c -lruntime -o start

count=1
if [ ! -z $1 ]; then
    count=$1
fi

arg_list=""
for (( i=1; i <= $count; i++ ))
do
    arg_list="$arg_list$(($i * 10)) "
done

./start -p $count $arg_list
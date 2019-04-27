#!/bin/bash
if [ -z $1 ]; then
    echo "write lab number"
    exit 1
fi

mkdir "pa$1"
cp *.h *c "pa$1"
tar -czvf "pa$1.tar.gz" "pa$1"/
rm -Rf "pa$1"/
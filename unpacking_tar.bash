#!/bin/bash
if [ -d "pa2_results/" ]; then
	rm -Rf "pa2_results"
fi
echo "12345" | gpg --passphrase-fd 0 -d pa2_results.tar.gz.gpg > pa2_results.tar.gz
tar -xvf pa2_results.tar.gz
rm pa2_results.tar.gz.gpg pa2_results.tar.gz


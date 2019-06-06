#!/bin/bash
if [ -d "pa$1_results/" ]; then
	rm -Rf "pa$1_results"
fi
echo "12345" | gpg --passphrase-fd 0 -d "pa$1_results.tar.gz.gpg" > "pa$1_results.tar.gz"
tar -xvf "pa$1_results.tar.gz"
rm "pa$1_results.tar.gz.gpg" "pa$1_results.tar.gz"


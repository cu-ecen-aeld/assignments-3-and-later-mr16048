#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Insufficient arguments"
    exit 1
fi

writefile=$1
writestr=$2

writeDir=$(dirname $writefile)
mkdir -p $writeDir

echo $writestr > $writefile
if [ $? -ne 0 ]; then
    echo "Error writing to file"
    exit 1
fi

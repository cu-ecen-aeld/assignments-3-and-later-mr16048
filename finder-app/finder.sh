#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Insufficient arguments."
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]; then
    echo "$filesdir   doesn not edxist or is not a directory."
    exit 1
fi

fileNum=0
lineNum=0

for file in "$filesdir"/*; do
    echo $file
    lineNumTemp=$(grep -c $searchstr $file)
    if [ $lineNumTemp > 0 ]; then
        fileNum=$(( $fileNum+1 ))
        lineNum=$(( $lineNum+$lineNumTemp ))
    fi
done

echo "The number of files are $fileNum and the number of matching lines are $lineNum."
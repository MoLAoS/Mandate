#!/bin/bash

# Copyright (C) 2010 by Nathan Turner (hailstone) for Glest Advanced Engine
# GPL V2, see source/licence.txt

if [ $# -lt 2 ]
then
	echo "Usage: $0 dir"
	exit 1
fi

# find the files
H_FILES=`find $1 -name *.h -type f -print`
CPP_FILES=`find $1 -name *.cpp -type f -print`

# count the lines for each file
lines=`wc -l $CPP_FILES $H_FILES`

# sort in reverse order
lines=`echo -e "$lines" | sort -r`

# sort and output top 20 largest line count, first line is the total
echo -e "$lines" | awk 'NR < 22 && NR > 1 { print }'
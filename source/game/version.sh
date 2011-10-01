#!/bin/bash

date_string=`date`
if [ -d "$1/.git" ]; then
	sha_string=`git --git-dir="$1/.git" describe`
else
	sha_string=''
fi

echo \#include \"version.h\" > version.cpp
echo "const char * build_date = \"$date_string\";" >> version.cpp
echo "const char * build_git_sha = \"$sha_string\";" >> version.cpp

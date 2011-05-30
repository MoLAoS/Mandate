#!/bin/bash

date_string=`date`
sha_string=`git describe`

echo \#include \"version.h\" > version.cpp
echo "const char * build_date = \"$date_string\";" >> version.cpp
echo "const char * build_git_sha = \"$sha_string\";" >> version.cpp

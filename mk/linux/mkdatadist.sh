#!/bin/bash

# This script compares current data with a Glest installation and creates a
# tarball containing all of the files that differ.

if [ "$1" = "process" ]; then
	shift
	dir="$1"
	shift
	while [ $# -gt 0 ]; do
		# Skip .svn, .git and CVS directories
		if ! echo "$1" | egrep '/(\.svn|\.git|CVS)/' > /dev/null; then
			diff -wq "$1" "$dir/$1" > /dev/null 2>&1 || echo $1
		fi
		shift
	done > ../../mk/linux/build/data_filelist.txt
	exit 0
fi

if [ ! -d "$1" ]; then
	echo "Usage: $0 <directory to Glest installation>"
	exit 1
fi
	
pushd ../../data/game > /dev/null &&
find data/core maps tilesets -type f -print0 | xargs -0 ../../mk/linux/mkdatadist.sh process "$1" &&
popd

set -x
eval tar czf build/data.tgz $(
	find -L gae -type f |
	egrep -v '/(\.svn|\.git|CVS)/'
) $(
	sed 's/^/\"/g; s/$/\"/g' build/data_filelist.txt
)
set +x

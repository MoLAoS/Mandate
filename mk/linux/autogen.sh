#!/bin/sh

# Correct working directory?
if test ! -f configure.ac ; then
  echo "*** Please invoke this script from directory containing configure.ac."
  exit 1
fi

# Perform cleanup activities
if [ -d build ]; then
	rm -rf build
fi

rm -f configure Jamconfig.in build aclocal.m4 \
	  data docs gae maps techs tilesets \
	  configurator g3d_viewer game map_editor shared_lib test

if [ "$1" = "clean" ]; then
	exit
fi


# Discover the branch to use as part of the build path, so that we can have
# multiple branches checked out & compiling at the same time.  If discovery
# fails (maybe the sources were downloaded rather than checked out of
# subversion) then just use "build".
svnRoot="https://glestae.svn.sourceforge.net/svnroot/glestae/"
branchSubDir="$(
	svn info |
	grep '^URL: ' |
	sed "s|^URL: ${svnRoot}||g; s|branches/||g; s|tags/|tag_|g" |
	awk -F/ '{print $1}' || echo "build"
)"
buildDir="/tmp/$(whoami || echo "build")/gae/${branchSubDir}"



echo "aclocal..."
#autoheader
aclocal -I mk/autoconf

# generate Jamconfig.in
echo "generating Jamconfig.in ..."
autoconf --trace=AC_SUBST \
  | sed -e 's/configure.ac:[0-9]*:AC_SUBST:\([^:]*\).*/\1 ?= "@\1@" ;/g' \
  > Jamconfig.in
sed -e 's/.*BACKSLASH.*//' -i~ Jamconfig.in
rm Jamconfig.in~ 
echo 'INSTALL ?= "@INSTALL@" ;' >> Jamconfig.in
echo 'JAMCONFIG_READ = yes ;' >> Jamconfig.in

echo "autoconf"
autoconf

rm -rf autom4te.cache

# Attempt to create build directory under /tmp file system (which should be
# tempfs on any modern *nix system) and if that fails, just create a
# sub-directory
if mkdir -p "${buildDir}"; then
	ln -s "${buildDir}" build
else
	mkdir build
fi

# create symlinks to the source dirs

echo "Updating Source symlinks..."

for f in data docs gae maps techs tilesets; do
	ln -sf ../../data/game/$f .;
done

for f in configurator g3d_viewer game map_editor shared_lib test; do
	ln -sf ../../source/$f .;
done

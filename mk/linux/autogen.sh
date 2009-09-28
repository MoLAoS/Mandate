#!/bin/sh

# Correct working directory?
if test ! -f configure.ac ; then
  echo "*** Please invoke this script from directory containing configure.ac."
  exit 1
fi

svnRoot="https://glestae.svn.sourceforge.net/svnroot/glestae/"
branchSubDir="$(
	svn info |
	grep '^URL: ' |
	sed "s|^URL: ${svnRoot}||g; s|branch/||g; s|tags/|tag_|g"
)"

rm -f data/lang/english.lng
rmdir data/lang > /dev/null 2>&1
rm -f configure Jamconfig.in build \
	  data docs gae maps techs tilesets \
	  configurator g3d_viewer game map_editor shared_lib test


if [ "$1" = "clean" ]; then
	exit
fi

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

rm -rf autom4te.cache build

mkdir -p /tmp/$(whoami)/gae/${branchSubDir}
ln -s /tmp/$(whoami)/gae/${branchSubDir} build

# create symlinks to the source dirs

echo "Updating Source symlinks..."

for f in data docs gae maps techs tilesets; do
	ln -sf ../../data/game/$f .;
done

for f in configurator g3d_viewer game map_editor shared_lib test; do
	ln -sf ../../source/$f .;
done

mkdir data/lang
ln -s ../../gae/data/lang/en.lng data/lang/english.lng

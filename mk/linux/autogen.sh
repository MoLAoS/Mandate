#!/bin/sh

rm -f configure Jamconfig.in

# Correct working directory?
if test ! -f configure.ac ; then
  echo "*** Please invoke this script from directory containing configure.ac."
  exit 1
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

mkdir -p /tmp/gae/0.2/build
ln -s /tmp/gae/0.2/build .

# create symlinks to the source dirs

echo "Updating Source symlinks..."

for f in data docs maps scenarios techs tilesets; do
	ln -sf ../../../data/glest_game/$f .;
done

if [ ! -d shared_lib ]; then
  ln -sf ../../source/shared_lib .
fi
if [ ! -d glest_game ]; then
  ln -sf ../../source/glest_game .
fi
if [ ! -d glest_map_editor ]; then
  ln -sf ../../source/glest_map_editor .
fi



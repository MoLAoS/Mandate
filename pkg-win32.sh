#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Usage $0 revision_or_version"
	exit
fi

archive=glestadv-win32-${1}.rar
rm -rf dist &&
mkdir dist &&
cd dist &&
mkdir -p data/lang data/core/menu/textures savegames &&
cp -p ../build/Release+SSE2/game/game.exe glestadv-sse2.exe &&
cp -p ../build/Release/game/game.exe glestadv-i686.exe &&
cp -p ../data/glest_game/glestadv.ini glestadv.ini &&
cp -p ../data/glest_game/data/lang/english.lng data/lang &&
cp -p ../data/glest_game/data/core/menu/menu.xml data/core/menu &&
cp -p ../data/glest_game/data/core/menu/textures/textentry.tga data/core/menu/textures &&
rar a -m5 -mdG -mt2 ${archive} glestadv-sse2.exe glestadv-i686.exe glestadv.ini data/lang/english.lng data/core/menu/menu.xml data/core/menu/textures/textentry.tga savegames &&
scp ${archive} root@beaker.javamonger.org:/svr/www/glest.codemonger.org/apache/htdocs/files/ &&
mv  ${archive} ../old


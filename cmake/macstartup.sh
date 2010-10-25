#!/bin/sh

GLEST_BUNDLE="`echo "$0" | sed -e 's|/Contents/MacOS/.*||'`"
GLEST_RESOURCES="$GLEST_BUNDLE/Contents/Resources"

export "DYLD_LIBRARY_PATH=$GLEST_RESOURCES/lib"
export "PATH=$GLEST_RESOURCES/bin:$PATH"

exec "$GLEST_RESOURCES/bin/glestadv" -datadir "$GLEST_RESOURCES/share/glestae"

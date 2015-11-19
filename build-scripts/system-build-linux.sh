#!/bin/sh

fail()
{
    echo "$1" 1>&2
    exit 1
}

BUILD_ARCHIVE_NAME="server"

# Clean and enter shadow build folder
echo Cleaning...
if [ -e ../build ]; then
    rm -Rf ../build || fail "Could not delete ../build"
fi

mkdir ../build || fail "Could not create ../build"
cd ../build || fail "Could not enter ../build"

# Run cmake
echo Running cmake...
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DUSE_SYSTEM_BOOST=ON     \
-DUSE_SYSTEM_FFMPEG=ON    \
-DUSE_SYSTEM_TBB=ON       \
-DUSE_SYSTEM_GLEW=ON      \
-DUSE_SYSTEM_FREETYPE=ON  \
-DUSE_SYSTEM_FREEIMAGE=ON \
-DUSE_SYSTEM_OPENAL=ON    \
-DUSE_SYSTEM_SFML=ON      \
-DUSE_SYSTEM_GTEST=ON     \
-DUSE_SYSTEM_FONTS=ON     \
.. || fail "cmake failed"

# Run make using the number of hardware threads in BUILD_PARALLEL_THREADS
echo Building...
time make -j${BUILD_PARALLEL_THREADS:-4} || fail "make failed"

# Create client folder to later zip
SERVER_FOLDER="$BUILD_ARCHIVE_NAME"
if [ -f "$SERVER_FOLDER" ]; then
    rm -Rf "$SERVER_FOLDER" || fail "Could not delete $SERVER_FOLDER"
fi
BIN_DIR="$SERVER_FOLDER/usr/bin"
LIB_DIR="$SERVER_FOLDER/usr/lib"
DOC_DIR="$SERVER_FOLDER/usr/share/doc/casparcg-server"
FONT_DIR="$SERVER_FOLDER/usr/share/fonts"

mkdir "$SERVER_FOLDER" || fail "Could not create $SERVER_FOLDER"
mkdir -p "$BIN_DIR"    || fail "Could not create $BIN_DIR"
mkdir -p "$LIB_DIR"    || fail "Could not create $LIB_DIR"
mkdir -p "$DOC_DIR"    || fail "Could not create $DOC_DIR"
mkdir -p "$FONT_DIR"   || fail "Could not create $FONT_DIR"

# Copy compiled binaries
echo Copying binaries...
cp -fa  shell/lib* "$LIB_DIR/" 2>/dev/null         || echo "Did not copy server libraries"
cp -fa  shell/*.ttf "$FONT_DIR/" 2>/dev/null       || echo "Did not copy fonts"
cp -fa  shell/casparcg "$BIN_DIR/casparcg-server"  || fail "Could not copy server executable"
cp -fa  shell/casparcg.config "$DOC_DIR/"          || fail "Could not copy server config"
cp -faR shell/locales "$BIN_DIR/" 2>/dev/null      || echo "Did not copy server CEF locales"

# Copy binary dependencies
echo Copying binary dependencies...
cp -fa  ../deploy/general/*.pdf "$DOC_DIR/"        || fail "Could not copy pdf"
cp -faR ../deploy/general/wallpapers "$DOC_DIR/"   || fail "Could not copy wallpapers"
cp -faR ../deploy/general/server/media "$DOC_DIR/" || fail "Could not copy media"

# Copy documentation
echo Copying documentation...
cp -fa  ../CHANGELOG "$DOC_DIR" || fail "Could not copy CHANGES.txt"
cp -fa  ../README    "$DOC_DIR" || fail "Could not copy README.txt"
cp -fa  ../LICENSE   "$DOC_DIR" || fail "Could not copy LICENSE.txt"

# Remove empty directories
rmdir "$LIB_DIR" 2>/dev/null
rmdir "$FONT_DIR" 2>/dev/null

# Create tar.gz file
echo Creating tag.gz...
tar -cvzf "$BUILD_ARCHIVE_NAME.tar.gz" "$SERVER_FOLDER" || fail "Could not create archive"

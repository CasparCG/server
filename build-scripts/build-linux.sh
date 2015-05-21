#!/bin/sh

fail()
{
    echo "$1" 1>&2
    exit 1
}

# Fail early if environment not set
[ -z "$BUILD_ARCHIVE_NAME" ] && fail "BUILD_ARCHIVE_NAME has to be set"
[ -z "$BUILD_PARALLEL_THREADS" ] && fail "BUILD_PARALLEL_THREADS has to be set"

# Clean and enter shadow build folder
echo Cleaning...
if [ -e ../build ]; then
    rm -Rf ../build || fail "Could not delete ../build"
fi

mkdir ../build || fail "Could not create ../build"
cd ../build || fail "Could not enter ../build"

# Run qmake
echo Running cmake...
cmake -G "Unix Makefiles" -A x64 -DCMAKE_BUILD_TYPE=RelWithDebInfo .. || fail "cmake failed"

# Run make using the number of hardware threads in BUILD_PARALLEL_THREADS
echo Building...
/usr/bin/time -f 'Build time %E' make -j$BUILD_PARALLEL_THREADS || fail "make failed"

# Create client folder to later zip
export SERVER_FOLDER="$BUILD_ARCHIVE_NAME"
if [ -f "$SERVER_FOLDER" ]; then
    rm -Rf "$SERVER_FOLDER" || fail "Could not delete $SERVER_FOLDER"
fi
mkdir "$SERVER_FOLDER" || fail "Could not create $SERVER_FOLDER"
mkdir "$SERVER_FOLDER/bin" || fail "Could not create $SERVER_FOLDER/bin"
mkdir "$SERVER_FOLDER/lib" || fail "Could not create $SERVER_FOLDER/lib"

# Copy compiled binaries
echo Copying binaries...
cp -fP shell/lib* "$SERVER_FOLDER/lib/" || fail "Could not copy server libraries"
cp -f shell/casparcg "$SERVER_FOLDER/bin/" || fail "Could not copy server executable"
cp -f shell/casparcg.config "$SERVER_FOLDER/" || fail "Could not copy server config"
cp -Rf shell/locales "$SERVER_FOLDER/bin/" || fail "Could not copy server CEF locales"

# Copy binary dependencies
echo Copying binary dependencies...
cp -RfP ../deploy/linux/* "$SERVER_FOLDER/" || fail "Could not copy binary dependencies"
#cp -RfP ../deploy/examples/* "$SERVER_FOLDER/" || fail "Could not copy binary dependencies"

# Copy documentation
echo Copying documentation...
cp -f ../CHANGES.txt "$SERVER_FOLDER/" || fail "Could not copy CHANGES.txt"
cp -f ../README.txt "$SERVER_FOLDER/" || fail "Could not copy README.txt"
cp -f ../LICENSE.txt "$SERVER_FOLDER/" || fail "Could not copy LICENSE.txt"

# Create tar.gz file
echo Creating tag.gz...
tar -cvzf "$BUILD_ARCHIVE_NAME.tar.gz" "$SERVER_FOLDER" || fail "Could not create archive"


#!/bin/bash

function usage()
{
    cat << EOU
Useage: bash $0 <path to the binary> <path to copy the dependencies>
EOU
exit 1
}

#Validate the inputs
[[ $# < 2 ]] && usage

#Check if the paths are vaild
[[ ! -e $1 ]] && echo "Not a vaild input $1" && exit 1
[[ -d $2 ]] || echo "No such directory $2 creating..."&& mkdir -p "$2"

#Get the library dependencies
export LD_LIBRARY_PATH=/opt/boost/lib
echo "Collecting the shared library dependencies for $1..."
deps=$(ldd $1 | awk 'BEGIN{ORS=" "}$1\
~/^\//{print $1}$3~/^\//{print $3}'\
 | sed 's/,$/\n/')
echo "Copying the dependencies to $2"

#Copy the deps
for dep in $deps
do
    if [[ $dep = *"ld-linux-x86-64.so"* ]] || [[ $dep = *"libc.so"* ]] || [[ $dep = *"libstdc++.so"* ]]; then
        echo "Skipping $dep"
    else
        echo "Copying $dep to $2"
        cp "$dep" "$2"
    fi
done

# Dynamic deps
cp "/usr/lib/x86_64-linux-gnu/nss/libsoftokn3.so" "$2"
cp "/usr/lib/x86_64-linux-gnu/nss/libnssckbi.so" "$2"

echo "Done!"

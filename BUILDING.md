Building the CasparCG Server
============================

The CasparCG Server source code uses the CMake build system in order to easily
generate build systems for multiple platforms. CMake is basically a build
system for generating build systems.

On Windows we can use CMake to generate a .sln file and .vcproj files. On
Linux CMake can generate make files or ninja files. Qt Creator has support for
loading CMakeLists.txt files directly.

Windows
=======

Development using Visual Studio
-------------------------------

1. Install Visual Studio 2017.

2. Install CMake (http://www.cmake.org/download/).

3. cmake -G "Visual Studio 14 2015" -A x64 ./src

4. Open "CasparCG Server.sln"

5. Build Solution

Linux
=====

Building inside Docker
----------------------

1. Checkout project
2. build-scripts/ubuntu-17.10/build-docker-image
3. build-scripts/ubuntu-17.10/launch-interactive
4. cmake /source
5. make -j8 (or however many cores you want to use)
6. /source/build-scripts/ubuntu-17.10/package

If all goes to plan, a package has been created in /build/products which should be accessible outside of the Docker container

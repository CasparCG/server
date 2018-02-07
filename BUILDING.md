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

3. `git clone --single-branch --branch 2.2.0 https://github.com/CasparCG/server`

4. `cmake -G "Visual Studio 15 2017" -A x64 ./server/src`

5. Open `./server/src/CasparCG Server.sln`

6. Build Solution

Linux
=====

Building inside Docker
----------------------

1. `git clone --single-branch --branch 2.2.0 https://github.com/CasparCG/server` casparcg-server-2.2.0
2. `cd casparcg-server-2.2.0`
3. `tools/linux/build-in-docker`

If all goes to plan, a docker image has been created containing CasparCG Server which can be run inside of a container.

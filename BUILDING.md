============================
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

This section is based on a Windows 7 Professional installation. Other versions
of Windows might also work.

Prerequisites:

1. Install Visual Studio 2017.

2. Install CMake (http://www.cmake.org/download/).

Development using Visual Studio
-------------------------------

1. cmake -G "Visual Studio 14 2015" -A x64 

2. Open CasparCG Server.sln

3. Right click on solution and choose "Restore NuGet Packages"

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

Legacy build instructions
-------------------------

All instructions are based on a clean install of the x86_64 version of Ubuntu
Desktop 14.04.3 LTS. Other distributions might work as well.

Prerequisites:

1. Install GCC, make and cmake etc:

   sudo apt-get install build-essential cmake

2. Install development packages for libraries which are not bundled:

   sudo apt-get install libxrandr-dev libjpeg-dev libsndfile1-dev libudev-dev libglu1-mesa-dev

Building a distribution
-----------------------

1. Enter the build-scripts folder.

2. Run ./set-variables-and-build-linux.sh (or edit it before running to change
   default settings). You can also set the environment variables manually and
   run ./build-linux.sh instead.

3. The distribution .tar.gz file can be found under build/

Building manually from command line
-----------------------------------

1. Start with unpacking dependencies64/large_files_linux.tar.xz:

   cd dependencies64
   tar xvJf large_files_linux.tar.xz

2. Create an empty directory called build in the source code root.

3. From the build directory type:

   cmake -G "Unix Makefiles" -A x64 -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

   ...for a release build or:

   cmake -G "Unix Makefiles" -A x64 -DCMAKE_BUILD_TYPE=Debug ..

   ...for a debug build.

4. A Makefile will have been generated allowing us to build using make:

   make -j12

   12 is the maximum number of parallel processes to compile via, and can be
   changed to a more suitable number for the machine doing the compilation
   (generally the number of hardware threads).

Development using Qt Creator
----------------------------

1. Start with unpacking dependencies64/large_files_linux.tar.xz:

   cd dependencies64
   tar xvJf large_files_linux.tar.xz

2. Install Qt Creator:

   sudo apt-get install qt-creator

3. Start Qt Creator and open CMake project CMakeLists.txt from the source code
   root.

4. Select where you want the shadow build to be performed. build/ directly
   under the source code root is recommended.

5. Under the build settings enter the following as "CMake arguments" for the
   "Release" build configuration:

   -DCMAKE_BUILD_TYPE=RelWithDebInfo

   ...and for the "Debug" build configuration set it to:

   -DCMAKE_BUILD_TYPE=Debug

6. For both "Release" and "Debug" build configurations expand the
   Build Steps - Details section and add -j12 to the "Additional arguments"
   (or any other number than 12 depending on the number of hardware threads
   in the computer). This will enable parallel building.

7. Under "Run Settings" check the "Run in Terminal" checkbox.

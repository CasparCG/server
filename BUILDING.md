# Building the CasparCG Server

The CasparCG Server source code uses the CMake build system in order to easily
generate build systems for multiple platforms. CMake is basically a build
system for generating build systems.

On Windows we can use CMake to generate a .sln file and .vcproj files. On
Linux CMake can generate make files or ninja files. Qt Creator has support for
loading CMakeLists.txt files directly.

# Windows

## Development using Visual Studio

1. Install Visual Studio 2019.

2. Install CMake (http://www.cmake.org/download/).

3. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`

4. `cd casparcg-server-master`

5. `mkdir build`

6. `cd build`

7. `cmake -G "Visual Studio 16 2019" -A x64 ../src`

8. Open `CasparCG Server.sln`

# Linux

## Building inside Docker

1. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`
2. `cd casparcg-server-master`
3. `./tools/linux/build-in-docker`

If all goes to plan, a docker image `casparcg/server` has been created containing CasparCG Server.

## Extracting CasparCG Server from Docker

1. `./tools/linux/extract-from-docker`

You will then find a folder called `casparcg_server` which should contain everything you need to run CasparCG Server.

_Note: if you ran docker with sudo, CasparCG server will not be able to run without sudo out of the box. For security reasons we do not recommend to run CasparCG with sudo. Instead you can use chown to change the ownership of the CasparCG Server folder._

## Development

1. Install Docker by following installation instructions from [Docker Docs][1]
2. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`
3. `cd casparcg-server-master`
4. Install dependencies, this can be done with `sudo ./tools/linux/install-dependencies`
5. Extract Boost, FFmpeg and CEF from the docker images via `sudo ./tools/linux/extract-deps-from-docker`. Alternatively these can be prepared manually by following the steps laid out in each Dockerfile
6. `mkdir build && cd build`
7. `cmake ../src`
8. `make -j8`

If all goes to plan, a folder called 'staging' has been created with everything you need to run CasparCG server.

[1]: https://docs.docker.com/install/linux/docker-ce/ubuntu/

## Build options

-DENABLE_HTML=OFF - useful if you lack CEF, and would like to build without that module.

-DUSE_SYSTEM_BOOST - (Linux only) use the version of Boost from your OS.

-DUSE_SYSTEM_FFMPEG - (Linux only) use the version of ffmpeg from your OS.

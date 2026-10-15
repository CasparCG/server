# Building the CasparCG Server

The CasparCG Server source code uses the CMake build system in order to easily
generate build systems for multiple platforms. CMake is basically a build
system for generating build systems.

On Windows we can use CMake to generate a .sln file and .vcproj files. On
Linux CMake can generate make files or ninja files. Qt Creator has support for
loading CMakeLists.txt files directly.

# Dependency caching

CMake will automatically download some dependencies as part of the build process.
These are taken from https://github.com/CasparCG/dependencies/releases (make sure to expand the 'Assets' group under each release to see the files), most of which are direct copies of distributions from upstream.

During the build, you can specify the CMake option `CASPARCG_DOWNLOAD_MIRROR` to download from an alternate HTTP server (such as an internally hosted mirror), or `CASPARCG_DOWNLOAD_CACHE` to use a specific path on disk for the local cache of these files, by default a folder called `external` will be created inside the build directory to cache these files.

If you want to be able to build CasparCG offline, you may need to manually seed this cache. You can do so by placing the correct tar.gz or zip into a folder and using `CASPARCG_DOWNLOAD_CACHE` to tell CMake where to find it.
You can figure out which files you need by looking at each of the `ExternalProject_Add` function calls inside of [Bootstrap_Linux.cmake](./src/CMakeModules/Bootstrap_Linux.cmake) or [Bootstrap_Windows.cmake](./src/CMakeModules/Bootstrap_Windows.cmake). Some of the ones listed are optional, depending on other CMake flags.

# Windows

## Building distributable

1. Install Visual Studio 2022.

2. Install 7-zip (https://www.7-zip.org/).

3. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`

4. `cd casparcg-server-master`

5. `.\tools\build.bat`

6. Copy the `dist\casparcg_server.zip` file for distribution

## Development using Visual Studio

1. Install Visual Studio 2022.

2. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`

3. Open the cloned folder in Visual Studio.

4. Build All and ensure it builds successfully

# Linux

## Building on your system

We only officially support Ubuntu LTS releases, other distros may work but often run into build issues. We are happy to accept PRs to resolve these issues, but are unlikely to write fixes ourselves.

We currently document two approaches to building CasparCG. The recommended way is to use the `deb` packaging we have in the repository for Ubuntu and Debian.
For native builds on other distributions, follow the Development steps below.

We also provide a script to produce a build in docker, but this is not recommended unless absolutely necessary. The resulting builds are often rather brittle depending on where they are used.

To perform a custom build, follow the Development steps below, and you may need to do some extra packaging steps, or install steps on the target systems.

### Building inside Docker

1. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`
2. `cd casparcg-server-master`
3. `./tools/linux/build-in-docker`

If all goes to plan, a docker image `casparcg/server` has been created containing CasparCG Server.

### Extracting CasparCG Server from Docker

1. `./tools/linux/extract-from-docker`

You will then find a folder called `casparcg_server` which should contain everything you need to run CasparCG Server.

_Note: if you ran docker with sudo, CasparCG server will not be able to run without sudo out of the box. For security reasons we do not recommend to run CasparCG with sudo. Instead you can use chown to change the ownership of the CasparCG Server folder._

## Development

Before beginning, check the build options section below, to decide if you want to use any to simplify or customise your build.
Install Git using your distribution's package manager before cloning.

1. `git clone --single-branch --branch master https://github.com/CasparCG/server casparcg-server-master`
2. `cd casparcg-server-master`
3. `sudo ./tools/linux/install-dependencies`
4. `./tools/build.sh` You can add any of the build options from below to this command.

If all goes to plan, a folder called `build/staging` has been created with everything you need to run CasparCG server on the build system.
The dependency installer requires root; the build itself should run as your normal user. The install tree is not a portable bundle: its system libraries and diagnostic font must also exist on the target machine.

The common build launcher calls the existing platform-specific tools: `tools/windows/build.bat` on Windows and `tools/linux/build-with-system-deps` on Linux.
On Linux, the installer selects APT, pacman, or zypper, and both scripts use `tools/linux/platform-config` to select the CEF provider and diagnostic font. No separate CEF setup step is required.

| Distribution | CEF source |
| --- | --- |
| Ubuntu 24.04 and 26.04 | CasparCG PPA, added by the installer |
| Debian 13 (Trixie) and Sid | Pinned CEF download |
| Arch Linux | Distribution `cef` package |
| openSUSE Leap 16.0 and Tumbleweed | Pinned CEF download |

HTML is enabled by default. These configurations target x86-64 and require CMake 3.28 or newer and Boost 1.83 or newer.
Ubuntu ARM64 requires the PPA's matching ARM64 CEF package; the downloaded CEF archive is x86-64 only.
The Arch installer performs a full pacman system upgrade. Non-Ubuntu targets are experimental.

On Linux, `BUILD_DIR` selects a different build directory and `PROC_COUNT` changes the default of two parallel build jobs.
Additional arguments are forwarded to CMake and override the script's defaults, for example:

```sh
PROC_COUNT=4 ./tools/build.sh -DCMAKE_BUILD_TYPE=Debug
```

To use downloaded CEF instead of Ubuntu's PPA or Arch's package, select it in both commands. Use a separate build directory when changing providers:

```sh
sudo ./tools/linux/install-dependencies --download-cef
BUILD_DIR=build-downloaded-cef ./tools/build.sh -DUSE_SYSTEM_CEF=OFF
```

Compatible packaged CEF is preferred. The checked Debian Trixie and openSUSE repositories do not provide a CEF development package, so these targets use the pinned binary archive.
The archive supplies prebuilt CEF libraries and resources; only `libcef_dll_wrapper` is compiled locally. Downloads are cached as described above.
To enable Vulkan, install a compatible Vulkan SDK and set up its environment before passing `-DENABLE_VULKAN=ON`. The Docker build already provisions its own SDK.

### Running Headless

CasparCG 2.5 and later can run without X11 using the normal Linux build. See the upstream [Headless Linux Server guide](https://github.com/CasparCG/casparcg.com/blob/master/docs/guides/linux-headless.md).
With `DISPLAY` unset, OpenGL uses EGL and CEF selects its headless platform. A working graphics driver is still required; do not configure screen output or graphical diagnostics. SFML remains a build dependency, so no separate headless build option is provided.

### Rocky Linux

Rocky Linux 10 meets the minimum Boost and CMake versions, but lacks SFML in its standard, CRB, EPEL, and RPM Fusion repositories.
The tools report this blocker without changing the system. Headless operation does not bypass the SFML build dependency, so Rocky builds and CI remain deferred.

## Build options

-DENABLE_HTML=OFF - useful if you lack CEF, and would like to build without that module.

-DUSE_STATIC_BOOST=ON - (Linux only, default OFF) statically link against Boost.

-DUSE_SYSTEM_CEF=OFF - (Linux only, CMake default ON) download the pinned CEF build instead of using system CEF. System CEF can come from the CasparCG Ubuntu PPA or a distro package providing `FindCEF.cmake` (such as Arch's). The native build script chooses the appropriate default per distribution.

-DENABLE_VULKAN=ON - (Linux only, default OFF) enable Vulkan support; requires a compatible Vulkan SDK.

-DENABLE_AVX2=ON (Linux only, default OFF) Enable the AVX and AVX2 instruction sets (requires a CPU that supports it)

-DDIAG_FONT_PATH - Specify an alternate path/font to use for the DIAG window. On linux, this will often want to be set to an absolute path of a font

-DCASPARCG_BINARY_NAME=casparcg-server - (Linux only) generate the executable with the specified name. This also reconfigures the install target to be a bit more friendly with system package managers.

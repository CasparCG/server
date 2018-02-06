## Updating CEF

It is structured so that the contents of the build zip can be extracted to the correct platform folder, and for some of the larger files to be added to relevent the large_files archive. 

Prebuilt binaries of official versions are available from http://opensource.spotify.com/cefbuilds/index.html where the minimal version should be chosen.

Before adding linux/Release/libcef.so, run the strip command on the file, to remove debugging symbols and drop the filesize to be more managable.

### Building
These steps build on the official steps from https://bitbucket.org/chromiumembedded/cef/wiki/MasterBuildQuickStart.md and https://bitbucket.org/chromiumembedded/cef/wiki/BranchesAndBuilding.md

#### Building Windows
1. Follow steps 1-6 from https://bitbucket.org/chromiumembedded/cef/wiki/MasterBuildQuickStart.md
1. Apply any patches to the CEF source
1. Follow steps 7-8 replacing Debug_GN_x86 with Release_GN_x64
1. Package the resulting binaries with the following
	```
	cd /path/to/chromium/src/cef/tools
	./make_distrib.bat --ninja-build --x64-build --minimal
	```
1. Copy the resulting archive from /path/to/chromium/src/cef/binary_distrib

#### Building Linux
1. Follow steps 1-7 from https://bitbucket.org/chromiumembedded/cef/wiki/MasterBuildQuickStart.md
1. Apply any patches to the CEF source
1. Follow steps 8-10 replacing Debug_GN_x64 with Release_GN_x64
1. Package the resulting binaries with the following
1. Package the resulting binaries with the following
	```
	cd /path/to/chromium/src/cef/tools
	./make_distrib.sh --ninja-build --x64-build --minimal
	```
1. Copy the resulting archive from /path/to/chromium/src/cef/binary_distrib

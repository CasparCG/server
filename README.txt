CasparCG Server 2.1 alpha
============================
Thank you for your interest in CasparCG Server, a professional software used to
play out and record professional graphics, audio and video to multiple outputs.
CasparCG Server has been in 24/7 broadcast production since 2006.

This release is considered untested and unstable, and is NOT intended for use in
professional production.

More information is available at http://casparcg.com/


COMMON SYSTEM REQUIREMENTS FOR WINDOWS AND LINUX 
------------------------------------------------
1. Intel processor capable of using SSSE3 instructions. 
   Please refer to
   http://en.wikipedia.org/wiki/SSSE3 for a complete list.
   While AMD processors might work, CasparCG Server has only been tested on
   Intel processors.

2. A graphics card (GPU) capable of OpenGL 3.0 is required.
   We strongly recommend that you use a separate graphics card, and avoid
   using the built-in GPU that exists in many CPUs. Your playback performance
   might suffer if using a built-in GPU.
   
   
SYSTEM REQUIREMENTS FOR WINDOWS
-------------------------------
1. 2. Windows 7 (64-bit) strongly recommended.
   CasparCG Server has also been used successfully on Windows 7 (32-bit) 
   and Windows XP SP2 (32-bit only.)
   NOT SUPPORTED: Windows 8, Windows 2003 Server and Windows Vista.
   
Microsoft Visual C++ 2010 Redistributable Package must be installed.
   See link in the Installation section below.
   
5. Microsoft .NET Framework 4.0 or later must be installed.
   See link in the Installation section below.
   
6. Windows' "Aero" theme and "ClearType" font smoothing must be disabled
   as they have been known to interfere with transparency in Flash templates,
   and can also cause problems with Vsync when outputting to computer screens.

The latest system recommendations are available at:
http://casparcg.com/wiki/CasparCG_Server#System_Requirements




INSTALLATION
------------

1. Check that your system meets the requirements above.

2. Unzip and place the "CasparCG Server" folder anywhere you like.

3. Install "Microsoft Visual C++ 2010 Redistributable Package" from
   http://www.microsoft.com/download/en/details.aspx?id=5555
   
4. Install "Microsoft .NET Framework" (version 4.0 or later) from
   http://go.microsoft.com/fwlink/?LinkId=225702

5. Make sure you turn off Windows' "Aero Theme" and "ClearType" font smoothing
   as they can interfere with CasparCG Server's OpenGL features!


INSTALLATION OF ADDITIONAL NON-GPL SOFTWARE
-------------------------------------------

- For Flash template support:

  1. Uninstall any previous version of the Adobe Flash Player using this file:
     http://download.macromedia.com/get/flashplayer/current/support/uninstall_flash_player.exe
  2. Download and unpack
     http://download.macromedia.com/pub/flashplayer/installers/archive/fp_11.8.800.94_archive.zip
  3. Install Adobe Flash Player 11.8.800.94 from the unpacked archive:
     fp_11.8.800.94_archive\11_8_r800_94\flashplayer11_8r800_94_winax.exe

- For NewTek iVGA support, please download and install the following driver:
  http://new.tk/NetworkSendRedist


CONFIGURATION
-------------
1. Configure the server by editing the self-documented casparcg.config file in a
   text editor.

2. Start the "casparcg.exe" program.

3. Connect to the Server from a client software, such as the "CasparCG Client"
   which is available as a separate download.


DOCUMENTATION
-------------
The most up-to-date documentation is always available at
http://casparcg.com/wiki/

Ask questions in the forum: http://casparcg.com/forum/


LICENSING
---------
CasparCG is distributed under the GNU General Public License GPLv3 or
higher, please see LICENSE.TXT for details.

The included software is provided as-is by Sveriges Televison AB.
More information is available at http://casparcg.com/

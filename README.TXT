Thank you for your interest in the CasparCG system! 
The included software is provided as-is by Sveriges Televison AB.

                                    
Fixes and New Features in CasparCG Server 1.8.2
===============================================
o Several bug fixes and stability enhancements.
o Now runs on 32-bit versions of Windows XP, Windows Vista and Windows 7. 
o Enhanced initialization of BlueFish cards.
o Support for 16 bit embedded audio on BlueFish cards.
o If two files in the Media folder share the same name, the .MOV file will be played out instead of the .TGA file.
o Support for HD output on DeckLink cards.
o External key is enabled by default on DeckLink cards.
o Support for playing compressed template files, .ct
o Support for scrolling TGA files with extension .stga


Known Issues
============
o No support for NTSC, only PAL SD + HD.
o Only one DeckLink card per machine is currently supported (multiple BlueFish cards supported.)
o No support for 64-bit Windows versions (but it runs on 32-bit versions.)
o Can't set RGB range to CBR on BlueFish Deepblue Lite Pro cards. Probably not supported by vendor on these cards.
o FFmpeg Producer can't do realtime scaling from SD to HD.


More information, samples and documentation 
===========================================
http://casparcg.com/
http://casparcg.com/forum/
http://casparcg.com/wiki/


Licensing
=========
This software is distributed under the GNU General Public 
License GPLv3 or higher, see the file COPYING.TXT for details. 

CasparCG uses libraries from the FFmpeg project under the 
GPLv2. FFmpeg is a trademark of Fabrice Bellard, originator 
of the FFmpeg project. Get more info about FFmpeg at: 
http://ffmpeg.org/

CasparCG uses the Threading Building Blocks library under
the GPLv2. Get more info on Threading Building Blocks at: 
http://www.threadingbuildingblocks.org/


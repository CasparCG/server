================================================================================
CasparCG Server 2.0.3
================================================================================

SYSTEM REQUIREMENTS
===================

1. Intel processor (while AMD processors should work, CasparCG Server has only
   been tested on Intel processors.)

2. We recommend Windows 7 (64-bit,) but it has also been successfully used on 
   Windows 7 (32-bit) and Windows XP SP2 (32-bit only.)
   
3. An OpenGL 3.0-capable graphics card from NVIDIA is required to run 
   CasparCG Server 2.0. Also make sure that you have installed the latest
   drivers.
   Please check your card's capabilities at: 
   http://en.wikipedia.org/wiki/Comparison_of_Nvidia_graphics_processing_units
   Graphics cards from other manufacturers _may_ work but have not been tested.

4. Microsoft Visual C++ 2010 Redistributable Package must be installed.
   Free download at:
   http://www.microsoft.com/download/en/details.aspx?id=5555

5. You must have Flash Player 10.3 or later installed (an installer can be found
   in the "Flash Player Installation" folder.)

The latest recommendations are available at:
http://casparcg.com/wiki/CasparCG_Server#System_Requirements


INSTALLATION
============

1. Check that your system meets the requirements.

2. Unzip and place the CasparCG Server 2.0.3 folder anywhere you like.

3. Install "Microsoft Visual C++ 2010 Redistributable Package" from
   http://www.microsoft.com/download/en/details.aspx?id=5555
   
4. Install "Flash Player 10.3.183.14" from the "Flash Player Installation"
   folder.

5. Make sure you turn off Windows' Aero Theme as it interferes with 
   CasparCG Server's OpenGL features!

6. Configure the server settings in the "casparcg.config" text file.

7. Start the "CasparCG.exe" program.

8. Connect to the server from a client, such as the included CasparCG 2.0 Demo
   Client (requires Adobe AIR.)


DOCUMENTATION
=============

The most up-to-date documentation is always available at
http://casparcg.com/wiki/


CONFIGURATION
=============

By default, CasparCG Server will look in the media folder for videos, audio and
images files. Flash templates are stored in the templates folder. If you want to
change the location (for example to a faster disk) you just change the paths in
the casparcg.config file.

* How to enable Screen consumer:

  Open casparcg.config and use the following node for consumers:

  <consumers>
      <screen/>
  </consumers>

* How to enable a DeckLink card and how to get video in and key output:

  Open casparcg.config and use the following node for consumers:

  <consumers>
      <decklink/>
  </consumers>

# Tip:

At the bottom of the casparcg.config file you will find comments which document
additional settings.


LICENSING
=========

CasparCG Server is distributed under the GNU General Public License GPLv3 or
higher, see LICENSE.TXT for details.


AUTHORS - People who have developed the CasparCG Server Software
==========(sorted alphabetically by last name)==================

Niklas 	Andersson   (Server 1.0-1.8)
Andreas	Jeansson    (Template-Host)
Robert 	Nagy        (Server 1.7-2.x)
Helge   Norberg     (Server 2.x)


CREDITS - People who have contributed to the CasparCG Project
==========(sorted alphabetically by last name)===============

Niklas  Andersson
Jonas   Hummelstrand    (jonas.hummelstrand@svt.se)
Andreas Jeansson        (andreas.jeansson@svt.se)
Peter   Karlsson        (peter.p.karlsson@svt.se)
Jeff    Lafforgue
Andy    Mace
Robert 	Nagy            (ronag89@gmail.com)
Helge   Norberg         (helge.norberg@svt.se)
Thomas  R. Kaltz III
Olle    Soprani

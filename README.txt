==========================================================================================================
CasparCG Video and Graphics Playout Server
==========================================================================================================

Thank you for your interest in CasparCG. The included software
is provided as-is by Sveriges Televison AB.

Please note that while the current CasparCG Client (version 1.8) will
continue to work with CasparCG Server 2.0, you won't be able to
trigger many of the new features. Please use the provided AIR app to
try out the new functionality or add the new features to you 
control application (you can find the documentation on the wiki or ask in the forums). 

==========================================================================================================
INSTALLATION
==========================================================================================================

You can place the CasparCG Server 2.0 folder anywhere you like, and even move it.

If you don't already have Flash installed, use the bundled version in the Flash folder

==========================================================================================================
CONFIGURATION
==========================================================================================================

By default, CasparCG Server will look in the media folder 
for videos, audio and images files. Flash templates are stored in the
templates folder. If you want to change the location (for example to a faster disk)
you just change the paths in the casparcg.config file.

* How to enable Screen consumer

Open casparcg.config and use the following node for consumers:

<consumers>
    <screen/>
</consumers>

* How to enable a DeckLink card and how to get video in and key output.

Open casparcg.config and use the following node for consumers:

<consumers>
    <decklink/>
</consumers>

# Tip:

At the bottom of the casparcg.config file you will find comments which document additional settings.

==========================================================================================================
LICENSING
==========================================================================================================


CasparCG is distributed under the GNU General Public License GPLv3 or higher, see COPYING.TXT for details.

==========================================================================================================
AUTHORS - People who have developed the CasparCG Server Software (sorted alphabetically by last name).
==========================================================================================================

Niklas 	Andersson   (Server 1.0-1.8)
Andreas	Jeansson    (Template-Host)
Robert 	Nagy        (Server 1.7-2.x)
	
==========================================================================================================
CREDITS - People who have contributed to the CasparCG Project (sorted alphabetically by last name).
==========================================================================================================

Niklas	Andersson
Jonas 	Hummelstrand    (jonas.hummelstrand@svt.se)
Andreas Jeansson        (andreas.jeansson@svt.se)
Peter 	Karlsson        (peter.p.karlsson@svt.se)
Robert 	Nagy            (robert.nagy@dfindredpatch.se, ronag89@gmail.com)
Olle 	Soprani 
Thomas 	R. Kaltz III 

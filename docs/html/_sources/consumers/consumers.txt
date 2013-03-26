#########
Consumers
#########

=============
File Consumer
=============

-----------
Diagnostics
-----------

ffmpeg[*filename*]

+---------------+-----------------------------------------------+--------+
| Graph         | Description                                   |  Scale |
+===============+===============================================+========+
| frame-time    | Time spent decoding the current frame.        | fps/2  |
+---------------+-----------------------------------------------+--------+
		
----------
Parameters
----------

The file consumer uses the same option syntax as ffmpeg. 

Note, not all options supported by ffmpeg have been implemented.

    
^^^^^^^^
FILENAME
^^^^^^^^

Target filename.

Syntax::

    [filename:string]
    
Example::

    ADD 1 FILE test.mov -vcodec libx264 -crf 5 -preset ultrafast -tune fastdecode -s 1280x720 -r 50 -acodec aac -ab 128k 
    REMOVE 1 FILE    
        
=================
Bluefish Consumer
=================

-----------
Diagnostics
-----------

ffmpeg[*filename*]

+---------------+-----------------------------------------------+--------+
| Graph         | Description                                   |  Scale |
+===============+===============================================+========+
| frame-time    | Time spent decoding the current frame.        | fps/2  |
+---------------+-----------------------------------------------+--------+
| sync-time     | Time spent waiting for sync.                  | fps/2  |
+---------------+-----------------------------------------------+--------+
| tick-time     | Time between frames frame.                    | fps/2  |
+---------------+-----------------------------------------------+--------+
		
----------
Parameters
----------

^^^^^^
DEVICE
^^^^^^

Which BlueFish device to attach.

Syntax::

	[device:int]
    
Configuration Syntax::

    <device>[1..]</device>
	
Example::
	
	<< ADD 1 BLUEFISH 1
    
    
^^^^^^^^^^^^^^
EMBEDDED_AUDIO
^^^^^^^^^^^^^^

Enables embedded-audio.

Syntax::

	EMBEDDED_AUDIO
    
Default::

    Disabled
    
Configuration Syntax::

    <embedded-audio>[true|false]</embedded-audio>
	
Example::
	
	<< ADD 1 BLUEFISH 1 EMBEDDED_AUDIO
    
^^^^^^^^
KEY_ONLY
^^^^^^^^

Displays key as fill.

Default::

    Disabled

Syntax::

	KEY_ONLY
    
Configuration Syntax::

    <key-only>[true|false]</key-only>
	    
Example::
	
	<< ADD 1 BLUEFISH 1 EMBEDDED_AUDIO KEY_ONLY
    

=================
Decklink Consumer
=================

-----------
Diagnostics
-----------

ffmpeg[*filename*]

+---------------+-----------------------------------------------+--------------+
| Graph         | Description                                   |  Scale       |
+===============+===============================================+==============+
| frame-time    | Time spent decoding the current frame.        | fps/2        |
+---------------+-----------------------------------------------+--------------+
| sync-time     | Time spent waiting for sync.                  | fps/2        |
+---------------+-----------------------------------------------+--------------+
| tick-time     | Time between frames frame.                    | fps/2        |
+---------------+-----------------------------------------------+--------------+
| dropped-frame | Frame was dropped.                            |  N/A         |
+---------------+-----------------------------------------------+--------------+
| late-frame    | Frame was late.                               |  N/A         |
+---------------+-----------------------------------------------+--------------+
| buffered-video| Frame was dropped.                            |  fps         |
+---------------+-----------------------------------------------+--------------+
| buffered-audio| Frame was late.                               |  cadence*2   |
+---------------+-----------------------------------------------+--------------+
		
----------
Parameters
----------

^^^^^^
DEVICE
^^^^^^

Which BlueFish device to attach.

Syntax::

	[device:int]
    
Configuration Syntax::

    <device>[1..]</device>
	
Example::
	
	<< ADD 1 BLUEFISH 1
    
    
^^^^^^^^^^^^^^
EMBEDDED_AUDIO
^^^^^^^^^^^^^^

Enables embedded-audio.

Syntax::

	EMBEDDED_AUDIO
    
Default::

    Disabled
    
Configuration Syntax::

    <embedded-audio>[true|false]</embedded-audio>
	
Example::
	
	<< ADD 1 BLUEFISH 1 EMBEDDED_AUDIO
    
^^^^^^^^
KEY_ONLY
^^^^^^^^

Displays key as fill.

Default::

    Disabled

Syntax::

	KEY_ONLY
    
Configuration Syntax::

    <key-only>[true|false]</key-only>
	    
Example::
	
	<< ADD 1 BLUEFISH 1 EMBEDDED_AUDIO KEY_ONLY    
    
^^^^^^^
LATENCY
^^^^^^^

Set latency mode.

Default::

    normal

Configuration Syntax::

    <latency>[normal|low|default]</latency>
        
^^^^^
KEYER
^^^^^

Set keyer mode.

Default::

    external

Configuration Syntax::

    <keyer>[external|internal|default]</keyer>
    
^^^^^^^^^^^^
BUFFER_DEPTH
^^^^^^^^^^^^

Set buffer depth. Settings this value to low can cause output distortion.

Default::

    3

Configuration Syntax::

    <buffer-depth>[1..]</buffer-depth>
    
===============
Screen Consumer
===============

=====================
System Audio Consumer
=====================
*****************
Bluefish Consumer
*****************

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
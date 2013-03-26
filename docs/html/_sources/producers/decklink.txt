*****************
Decklink Producer
*****************

-----------
Diagnostics
-----------

flash[*model-name* | *device-index* | *video-mode*]

+---------------+-----------------------------------------------+--------+
| Graph         | Description                                   |  Scale |
+===============+===============================================+========+
| frame-time    | Time spent rendering the current frame.       | fps/2  |
+---------------+-----------------------------------------------+--------+
| tick-time     | Time between rendering two frames.            | fps/2  |
+---------------+-----------------------------------------------+--------+
| dropped-frame | Dropped an input frame.                       |  N/A   |
+---------------+-----------------------------------------------+--------+
| late-frame    | Frame was not ready in time and is skipped.   |  N/A   |
+---------------+-----------------------------------------------+--------+
| output-buffer | Buffering.                                     |        |
+---------------+-----------------------------------------------+--------+
----------
Parameters
----------

^^^^^^
DEVICE
^^^^^^

Which BlackMagic device to attach.

Syntax::

	[device:int]
	
Example::
	
	<< PLAY 1-1 DECKLINK 1
    
^^^^^^
LENGTH
^^^^^^
Sets the end of the file.

Syntax::

	LENGTH [frames:int]
	
Example::
	
	<< PLAY 1-1 DECKLINK 1 LENGTH 100
	
^^^^^^
FILTER
^^^^^^
Configures libavfilter which will be used.

Syntax::

	FILTER [libavfilter-parameters:string]
		
Example::
		
	<< PLAY 1-1 DECKLINK 1 FILTER hflip:yadif=0:0
	
^^^^^^
FORMAT
^^^^^^
Sets the video-mode. If no video-mode is provided then the parent channels video-mode will be used.

Syntax::

	FORMAT [video-mode:string]
	
Example::
	
	<< PLAY 1-1 DECKLINK 1 FORMAT PAL LENGTH 100
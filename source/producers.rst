#########
Producers
#########

======
ffmpeg
======

-------
Filters
-------

The ffmpeg producer supports "libavfilter" filters through the "FILTER" parameter.

-----------
Diagnostics
-----------

ffmpeg[ *filename* | *video-mode* | *file-frame-number* / *file-nb-frames*]

+---------------+-----------------------------------------------+--------+
| Graph         | Description                                   |  Scale |
+===============+===============================================+========+
| frame-time    | Time spent decoding the current frame.        | fps/2  |
+---------------+-----------------------------------------------+--------+
| buffer-count  | Number of input packets buffered.             |  100   |
+---------------+-----------------------------------------------+--------+
| buffer-count  | Size of buffered input packets.               | 16MB   |
+---------------+-----------------------------------------------+--------+
| underflow     | Frame was not ready in time and is skipped.   |  N/A   |
+---------------+-----------------------------------------------+--------+
| seek          | Input has seeked.                             |  N/A   |
+---------------+-----------------------------------------------+--------+
		
----------
Parameters
----------

^^^^
LOOP
^^^^
Sets whether file will loop.

Syntax::

	{LOOP}
	
Example::
	
	PLAY 1-1 MOVIE LOOP
	
^^^^
SEEK
^^^^
Sets the start of the file. This point will be used while looping.

Syntax::

	SEEK [frames:int]
	
Example::
	
	PLAY 1-1 MOVIE SEEK 100 LOOP
	
^^^^^^
LENGTH
^^^^^^
Sets the end of the file.

Syntax::

	LENGTH [frames:int]
	
Example::
	
	PLAY 1-1 MOVIE LENGTH 100
	
^^^^^^
FILTER
^^^^^^
Configures libavfilter which will be used.

Syntax::

	FILTER [libavfilter-parameters:string]
		
Example::
		
	PLAY 1-1 MOVIE FILTER hflip:yadif=0:0
	
---------
Functions
---------

^^^^
LOOP
^^^^
Sets whether file will loop. 

Syntax::

	LOOP [loop:0|1]
	
Returns

	The value of LOOP after the command have completed.
	
Example::
	
	CALL 1-1 LOOP 1
	CALL 1-1 LOOP   // Queries without changing.
	
^^^^
SEEK
^^^^
Seeks in the file.

Syntax::

	SEEK [frames:int]
	
Returns

	Nothing.
	
Example::
	
	CALL 1-1 SEEK 200
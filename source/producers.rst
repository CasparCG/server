#########
Producers
#########

======
ffmpeg
======

--------------------
Supported containers
--------------------

+---------------------------------------+
| mpg, mpeg, m2v, m4v, mp3, mp4, mpga   |
+---------------------------------------+
| avi                                   | 
+---------------------------------------+
| mov                                   | 
+---------------------------------------+
| webmv                                 | 
+---------------------------------------+
| f4v, flv                              | 
+---------------------------------------+
| mkv, mka                              | 
+---------------------------------------+
| dv                                    | 
+---------------------------------------+
| wmv, wma, wav                         | 
+---------------------------------------+
| rm, ram                               | 
+---------------------------------------+
| ogg, ogv, oga, ogx                    | 
+---------------------------------------+
| divx, xvid                            | 
+---------------------------------------+

--------------
Deinterlaceing
--------------

-------
Filters
-------

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

	LOOP (?<frame>0|1)
	
^^^^
SEEK
^^^^
Sets the start of the file. This point will be used while looping.

Syntax::

	SEEK (?<frames>\d+)
	
Example::
	
	PLAY 1-1 MOVIE SEEK 100
	
^^^^^^
LENGTH
^^^^^^
Sets the end of the file.

Syntax::

	LENGTH (?<frames>\d+)
	
Example::
	
	PLAY 1-1 MOVIE LENGTH 100
	
^^^^^^
FILTER
^^^^^^
Configures as libavfilter which will be used for the file.

Syntax::

	FILTER (?<libavfilter-parameters>[\d\w]+)
		
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

	LOOP (?<frame>0|1)?
	
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

	SEEK (?<frames>\d+)
	
Returns

	Nothing.
	
Example::
	
	CALL 1-1 SEEK 200
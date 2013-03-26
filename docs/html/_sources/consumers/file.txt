*****************
File Consumer
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
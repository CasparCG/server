.. _tut-file:

*************************
Running the File Consumer
*************************

The file consumer uses ffmpeg to encode video.

The encoding will automatically take advantage of multi-core CPUs.

To start the file consumer you to send the following command:

::
    
    ADD 1 FILE myfile.mov

If the "-f"  or "-vcodec/-acodec" option is not supplied then the container format and appropriate codec will be automatically deduced from the file extension.

To stop writing to the file send:

::

    REMOVE 1 FILE


The file consumer follows commandline arguments syntax used by ffmpeg, see ffmpeg for more options. Some of the available options are:

::

    -f // container format
    -vcodec // vicdeo codec
    -pix_fmt // pixel_format
    -r // video framerate
    -s // size
    -b // video bitrate
    -acodec // audio codec
    -ar // audio samplerate
    -ab // audio bitrate
    -ac // audio channels

::
        
    ADD 1 FILE myfile.mov -vcodec dnxhd
    ADD 1 FILE myfile.mov -vcodec prores
    ADD 1 FILE myfile.mov -vcodec dvvideo
    ADD 1 FILE myfile.mov -vcodec libx264

For the above formats have already provided high quality default parameters. 

Another example:

::

    ADD 1 FILE myfile.mov -vcodec libx264 -preset ultrafast -tune fastdecode -crf 5
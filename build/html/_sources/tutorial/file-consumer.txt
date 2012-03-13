.. _tut-file:

*************************
Running the File Consumer
*************************

The disk consumer uses ffmpeg to encode video and ffmpeg users will feel somewhat at home with the commands.

The encoding will take advantage of multi-core cpus.

To start the disk consumer you need to send the following command:

::
    
    ADD 1 FILE myfile.mov

Where the file extension will decide the container format and appropriate codec.

To stop it you simply type:

::

    REMOVE 1 FILE

The default, and recommended format is H264 which uses the highly optimized libx264 encoder. You can of course also choose what codec to encode to. The file consumer follows commandline arguments syntax used by ffmpeg, see ffmpeg for more options. Some of the available options are:

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

For the above formats we have already provided high quality default parameters. Though you can of course also specify other codecs (see the -vcodec option in ffmpeg), and the corresponding options (see ffmpeg documentation):

::

    ADD 1 FILE myfile.mov -vcodec libx264 -preset ultrafast -tune fastdecode -crf 5
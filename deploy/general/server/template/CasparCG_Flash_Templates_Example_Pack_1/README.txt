
Thank you for your interest in CasparCG Flash Template Example Pack 1. 
The included software is provided as-is by Sveriges Televison AB.


This example pack contains 4 different templates that demonstrates some of the possibilities of using Flash templates in CasparCG.

To try the .ft files you will need the CasparCG Server and the CasparCG Client from http://www.casparcg.com/download

To change and/or build the .ft files you will need the "CasparCG FT Generator" (which can be found on the same site) plus a copy of Adobe Flash Professional CS4 or later.

Documentation can be found at http://www.casparcg.com/wiki

These templates are for educational/testing purpose only and should not be used as is in production (unless you know what you are doing.)

*** SimpleTemplate1***
This is a simple template that will produce a programme identifier in the top left of the screen. There is one label, "swell" that will create a swell effect on the identifier. This can be played if there is an Invoke command with the parameter "swell" or if there is a Next command (Step) when the template is on the swell keyframe (it will reach that point when the intro animation is done). The template takes no data.


***SimpleTemplate2***
This is a simple template that has two dynamic text fields that will be accessible from the CasparCG Client by using their names "field1" and "field2". The template also has a video background that the text fields adapts to by fading up when the videos intro is done. If you run this template with any of the other templates playing you will notice that it will not play very smooth. This is not a performance issue but this template uses 25 fps and if there is any template loaded that uses 50 fps CasparCG will use the higher frame rate.


***AdvancedTemplate1***
This template will count down from a specified amount of seconds in the MM:SS format. When it reaches 0 it will pause for 3 seconds and then remove itself.  It can also load an image, you can use the very ugly "time.png" which is included in this example package. It takes the parameter "time" which is the time in seconds. It also takes the parameter "image" which is the path to the image. This path is always relative to where your CG.fth file is located. If you put it in the same folder as this file it is enough to write time.png as the value of "image". This template uses the Document Class "AdvancedTemplate1.as".


***AdvancedTemplate2***
This template will communicate with other instances of the template via the communicationManager.SharedData module. It will always choose the top position when played and if there is another instance of the template on this position that instance will move to the lower postition. If the lower position is occupied by a third instance of the template that instance will be stopped. It has one text field that can be adressed and it is called "field1". To test this system open up your CasparCG client and choose this template. Enter some data for field1 and load and play on layer 0. Change the layer to 1 and change the data if you like to and press load play. You should notice that the first instance will move down to give room to the second instance. If you load play on layer 2 the first instance should do an outro animation and disappear and then the second template moves down to give room to the third. This template uses the Document Class "AdvancedTemplate2.as".



LICENSING
This software is distributed under the GNU General Public 
License GPLv3 or higher, see the file COPYING.TXT for details. 
More information, samples and documentation at: 
http://casparcg.com/
http://casparcg.com/forum/
http://casparcg.com/wiki/


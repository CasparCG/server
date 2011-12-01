########
Commands
########

********
amcp 2.0
********

The Advanced Media Control Protocol (AMCP) is the main communication protocol used to control and query CasparCG Server 2.0.

* All communication is presumed to be encoded in UTF-8.
* Each command has to be terminated with both a carriage return and a linefeed character. For example:
	* \r\n
	* <CR><LF>
	* <0x0D><0x0A>
	* <13><10>
* The whole command string is case insensitive.
* Since the parameters in a command is separated by spaces, you need to enclose the parameter with quotation marks if you want it to contain spaces.

=======================
Backwards Compatibility
=======================

The AMCP 2.0 protocol implementation is mostly backward compatible with the previous CasparCG 1.7.1 AMCP Protocol and CasparCG 1.8.0 AMCP Protocol. This is achieved by providing default values for parameters used by the AMCP 2.0 protocol.

----------------
Breaking Changes
----------------

* The ''CLEAR'' command will also clear any visible template graphic in the specified container.

=================
Special sequences
=================

Since bare quotation marks are used to keep parameters with spaces in one piece, there has to be another way to indicate a quotation mark in a string. Enter special sequences. They behave as in most programming languages. The escape character is the backslash \ character. In order to get a quotation mark you enter \" in the command.
Valid sequences:

* \\" Quotation mark
* \\\\ Backslash
* \\n New line	

These sequences apply to all parameters, it doesn\'t matter if it\'s a file name or a long string of xml-data.

=====================
Command Specification
=====================

Commands are documented as regular expression with the extension of {} which indicate default values for optional parameters.

----------------
Basic Commands
----------------

^^^^^^
LOADBG
^^^^^^
Loads a producer in the background and prepares it for playout.
If no layer is specified the default layer index will be used.

Syntax:: 

	LOADBG 
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}
		(?<producer>[\d\w]+) 
		(
			(?<transitions>CUT|MIX|PUSH|WIPE|SLIDE) 
			(?<duration>\d+)
			(?<tween>(linear|easein|and-more))?{LINEAR}
			(?<direction>LEFT|RIGHT)?{RIGHT}
		)?{CUT 0}
		(?<auto>AUTO)?{}
		(?<parameters>.*)?{}
		
Example::

	LOADBG 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
		
^^^^
LOAD
^^^^
Loads a producer to the foreground and displays the first frame.
If no layer is specified the default layer index will be used.

Syntax:: 

	LOAD
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}
		(?<producer>[\d\w]+) 
		(
			(?<transitions>CUT|MIX|PUSH|WIPE|SLIDE) 
			(?<duration>\d+)
			(?<tween>(linear|easein|and-more))?{LINEAR}
			(?<direction>LEFT|RIGHT)?{RIGHT}
		)?{CUT 0}
		(?<auto>AUTO)?{}
		(?<parameters>.*)?{}
	
Example::	

	LOAD 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
	
^^^^
PLAY
^^^^	
Moves producer from background to foreground and starts playing it. If a transition (see LOADBG) is prepared, it will be executed.
If additional parameters (see LOADBG) are provided then the provided producer will first be loaded to the background.
If no layer is specified the default layer index will be used.

Syntax::

	PLAY
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}
		(?<producer>[\d\w]+) 
		(
			(?<transitions>CUT|MIX|PUSH|WIPE|SLIDE) 
			(?<duration>\d+)
			(?<tween>(linear|easein|and-more))?{LINEAR}
			(?<direction>LEFT|RIGHT)?{RIGHT}
		)?{CUT 0}
		(?<auto>AUTO)?{}
		(?<parameters>.*)?{}
	
Example::

	PLAY 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
	PLAY 1-1
	
^^^^^
PAUSE
^^^^^
Pauses foreground clip.

Syntax::	

	PAUSE 
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}	

Example::

	PAUSE 1-1
	
^^^^^
STOP
^^^^^
Removes foreground clip. If no layer is specified the default layer index will be used.

Syntax::	

	STOP 
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}	

Example::

	STOP 1-1

^^^^^
CLEAR
^^^^^
Removes both foreground and background clips. If no layer is specified then all layers in the specified video-channel are cleared.

Syntax::	

	CLEAR 
		(?<video-channel>\d+)
		(-(?<layer>\d+))?{0}	

Example::

	CLEAR 1-1
		
^^^^^^
CALL
^^^^^^
Calls a producers specific function.

Syntax::

	CALL 
		(?<video-channel>\d+
		(-(?<layer>\d+))?{0}	
		(?<function>[\d\w]+) 
		(?<parameters>.*)?{}

Example::

	CALL 1-1 SEEK 400
		
^^^^
SWAP
^^^^
Swaps layers between channels (both foreground and background will be swapped). If layers are not specified then all layers in respective video-channel will be swapped.

Syntax::

	SWAP 
		(?<video-channel1>\d+)
		(-(?<layer1>\d+))?		
		(?<video-channel2>\d+)
		(-(?<layer2>\d+))? 

Example::

	SWAP 1-1 1-2
	SWAP 1-0 2-0		
		
^^^
ADD
^^^
Adds consumer to output.

Syntax::

	ADD 
		(?<video-channel1>\d+) 
		(?<consumer>[\d\w]+) 
		(?<parameters>.*)
	
Example::

	ADD 1 FILE output.mov CODEC DNXHD
	ADD 1 DECKLINK DEVICE 1
		
^^^^^^
REMOVE
^^^^^^
Removes consumer from output.

Syntax::

	REMOVE 
		(?<video-channel1>\d+) 
		(?<consumer>[\d\w]+) 
		(?<parameters>.*)

Example::

	REMOVE 1 FILE 
	REMOVE 1 DECKLINK DEVICE 1
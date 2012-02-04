**************
Basic Commands
**************

======
LOADBG
======
Loads a producer in the background and prepares it for playout.
If no layer is specified the default layer index will be used.

Syntax:: 

	LOADBG [channel:int]-[layer:int] [clip:string] {[transition:CUT|MIX|PUSH|WIPE|SLIDE] [duration:int] {tween:string} {direction:LEFT|RIGHT} {auto:AUTO} {parameters:string}}
			
Example::

	<<< LOADBG 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
		
====
LOAD
====
Loads a producer to the foreground and displays the first frame.
If no layer is specified the default layer index will be used.

Syntax:: 

	LOAD [channel:int]-[layer:int] [clip:string] {[transition:CUT|MIX|PUSH|WIPE|SLIDE] [duration:int] {tween:string} {direction:LEFT|RIGHT} {auto:AUTO} {parameters:string}}
	
Example::	

	<<< LOAD 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
	
====
PLAY
====	
Moves producer from background to foreground and starts playing it. If a transition (see LOADBG) is prepared, it will be executed.
If additional parameters (see LOADBG) are provided then the provided producer will first be loaded to the background.
If no layer is specified the default layer index will be used.

Syntax::
	
	PLAY [channel:int]-[layer:int] [clip:string] {[transition:CUT|MIX|PUSH|WIPE|SLIDE] [duration:int] {tween:string} {direction:LEFT|RIGHT} {auto:AUTO} {parameters:string}}
	
Example::

	<<< PLAY 1-1 MY_VIDEO PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip 
	<<< PLAY 1-1
	
=====
PAUSE
=====
Pauses foreground clip.

Syntax::	

	PAUSE [channel:int]-[layer:int]

Example::

	<<< PAUSE 1-1
	
=====
STOP
=====
Removes foreground clip. If no layer is specified the default layer index will be used.

Syntax::	

	STOP [channel:int]-[layer:int]

Example::

	<<< STOP 1-1

=====
CLEAR
=====
Removes both foreground and background clips. If no layer is specified then all layers in the specified video-channel are cleared.

Syntax::	

	CLEAR [channel:int]{-[layer:int]}

Example::

	<<< CLEAR 1-1
	<<< CLEAR 1
		
======
CALL
======
Calls a producers specific function.

Syntax::

	CALL [channel:int]-[layer:int] [function:string] {parameters:string}

Example::

	<<< CALL 1-1 SEEK 400
		
====
SWAP
====
Swaps layers between channels (both foreground and background will be swapped). If layers are not specified then all layers in respective video-channel will be swapped.

Syntax::

	SWAP [channel:int]{-[layer:int]} [channel:int]{-[layer:int]}

Example::

	<<< SWAP 1-1 1-2
	<<< SWAP 1-0 2-0		
		
===
ADD
===
Adds consumer to output.

Syntax::

	ADD [channel:int] [consumer:string] {parameters:string}
	
Example::

	<<< ADD 1 FILE output.mov CODEC DNXHD
	<<< ADD 1 DECKLINK 1
		
======
REMOVE
======
Removes consumer from output.

Syntax::

	REMOVE [channel:int] [consumer:string] {parameters:string}

Example::

	<<< REMOVE 1 FILE 
	<<< REMOVE 1 DECKLINK 1
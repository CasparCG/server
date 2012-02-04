**************
Mixer Commands
**************

===========
MIXER KEYER
===========

If *keyer* equals 1 then the specified layer will not be rendered, instead it will be used as the key for the layer above.

Syntax::

	MIXER [channel:int]-[layer:int] KEYER [keyer:0|1]
		
Example::

	<<< MIXER 1-1 KEYER 1
	
===========
MIXER BLEND
===========

Syntax::

	MIXER
		[channel:int]-[layer:int] BLEND [blend-mode:string]
		
Example::

	<<< MIXER 1-1 BLEND overlay
	
See:: 

Blend-modes.
	
=============
MIXER OPACITY
=============

Syntax::

	MIXER
		[channel:int]-[layer:int] OPACITY [opacity:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 OPACITY 0.5
	
================
MIXER BRIGTHNESS
================

Syntax::

	MIXER [channel:int]-[layer:int] BRIGTHNESS [brightness:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 BRIGTHNESS 0.5
	
================
MIXER SATURATION
================

Syntax::

	MIXER
		[channel:int]-[layer:int] SATURATION [saturation:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 SATURATION 0.5
	
==============
MIXER CONTRAST
==============

Syntax::

	MIXER
		[channel:int]-[layer:int] CONTRAST [contrast:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 CONTRAST 0.5

============
MIXER LEVELS
============

Syntax::

	MIXER [channel:int]-[layer:int] SATURATION [min-input:double] [max-input:double] [gamma:double] [min-output:double] [max-output:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 LEVELS 0.1 0.1 1.0 0.9 0.9
	
==========
MIXER FILL
==========
Transforms the video stream on the specified layer.

Syntax::

	MIXER [channel:int]-[layer:int]	FILL [x:double] [y:double] [x-scale:double] [y-scale:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 FILL 0.25 0.25 0.5 0.5
	
==========
MIXER CLIP
==========
Masks the video stream on the specified layer.

Syntax::

	MIXER [channel:int]-[layer:int] [x:double] [y:double] [x-scale:double] [y-scale:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 CLIP 0.25 0.25 0.5 0.5
	
==========
MIXER GRID
==========
Creates a grid of video streams in ascending order of the layer index, i.e. if resolution equals 2 then a 2x2 grid of layers will be created.

	MIXER [channel:int] RID	[resolution:int]
		
Example::

	<<< MIXER 1 GRID 2

============
MIXER VOLUME
============
Changes the volume of the specified layer.

Syntax::

	MIXER [channel:int]-[layer:int] VOLUME [volume:double] {[tween:string] [duration:int]}
		
Example::

	<<< MIXER 1-1 VOLUME 0.5
	
===========
MIXER CLEAR
===========

Syntax::

	MIXER [channel:int]-[layer:int]	CLEAR
		
Example::

	<<< MIXER 1-1 CLEAR
		
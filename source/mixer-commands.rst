**************
MIXER Commands
**************

===========
MIXER KEYER
===========

If *keyer* equals 1 then the specified layer will not be rendered, instead it will be used as the key for the layer above.

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		KEYER
		(?<keyer>(0|1))
		
Example::

	<<< MIXER 1-1 KEYER 1
	
===========
MIXER BLEND
===========

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		BLEND
		(?<blend-mode>(0|1))
		
Example::

	<<< MIXER 1-1 BLEND overlay
	
=============
MIXER OPACITY
=============

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		OPACITY
		(?<opacity>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 OPACITY 0.5
	
================
MIXER BRIGTHNESS
================

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		BRIGTHNESS
		(?<brightness>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 BRIGTHNESS 0.5
	
================
MIXER SATURATION
================

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		SATURATION
		(?<saturation>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 SATURATION 0.5
	
==============
MIXER CONTRAST
==============

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		SATURATION
		(?<contrast>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 CONTRAST 0.5

============
MIXER LEVELS
============

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		SATURATION
		(?<min-input>(\d*\.?\d+))
		(?<max-input>(\d*\.?\d+))
		(?<gamma>(\d*\.?\d+))
		(?<min-output>(\d*\.?\d+))
		(?<max-output>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 LEVELS 0.1 0.1 1.0 0.9 0.9
	
==========
MIXER FILL
==========
Transforms the video stream on the specified layer.

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		FILL
		(?<x>(\d*\.?\d+))
		(?<y>(\d*\.?\d+))
		(?<x-scale>(\d*\.?\d+))
		(?<y-scale>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 FILL 0.25 0.25 0.5 0.5
	
==========
MIXER CLIP
==========
Masks the video stream on the specified layer.

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		CLIP
		(?<x>(\d*\.?\d+))
		(?<y>(\d*\.?\d+))
		(?<x-scale>(\d*\.?\d+))
		(?<y-scale>(\d*\.?\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 CLIP 0.25 0.25 0.5 0.5
	
==========
MIXER GRID
==========
Creates a grid of video streams in ascending order of the layer index, i.e. if resolution equals 2 then a 2x2 grid of layers will be created.

	MIXER
		(?<video-channel>[1-9]+)
		GRID
		(?<resolution>(\d+))
		
Example::

	<<< MIXER 1 GRID 2

============
MIXER VOLUME
============
Changes the volume of the specified layer.

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		VOLUME
		(?<volume>(\d+))
		(
			(?<tween>\w+)
			(?<duration>\d+)
		)?
		
Example::

	<<< MIXER 1-1 VOLUME 0.5
	
===========
MIXER CLEAR
===========

Syntax::

	MIXER
		(?<video-channel>[1-9]+)(-(?<layer>\d+))? 
		CLEAR
		
Example::

	<<< MIXER 1-1 CLEAR
		
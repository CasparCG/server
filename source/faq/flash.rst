=========
Flash FAQ
=========

Is Flash GPU acceleration supported?
------------------------------------

CasparCG does not support GPU acceleration for Flash. 


Is Stage3D supported?
---------------------

Yes, but only using the CPU fallback renderer.

Why isn't Flash audio embedded into the SDI signal?
---------------------------------------------------

CasparCG is currently unable to capture Flash audio since Flash sends audio directly to the system default audio device. 
There have been some successful in capture flash audio using hooks, however work on this issue have been prioritized.
**************************
Template Graphics Commands
**************************

Data to templates is sent as xml, formatted as follows::

	<templateData> 
		<componentData id="f0"> 
			<data id="text" value="Niklas P Andersson" /> 
		</componentData> 
		<componentData id="f1"> 
		<data id="text" value="Developer" /> 
		</componentData> 
		<componentData id="f2"> 
			<data id="text" value="Providing an example" /> 
		</componentData> 
	</templateData>
	
The node under each componentData is sent directly into the specified component. 
This makes it possible to provide completely custom data to templates. 
The data-nodes in this example is just the way the default CasparCG textfield wants its data. 
More information about this will be provided with the tools and ActionScript classes required to build your own templates.
A complete call to CG ADD (see below), correctly escaped and with the data above would look like this::

	CG 1 ADD 0 "demo/test" 1 "<templateData><componentData id=\"f0\"><data id=\"text\" value=\"Niklas P Andersson\"></data> </componentData><componentData id=\"f1\"><data id=\"text\" value=\"developer\"></data></componentData><componentData id=\"f2\"><data id=\"text\" value=\"Providing an example\"></data> </componentData></templateData>"

======
CG ADD
======

Prepares a template for displaying. It won't show until you call CG PLAY (unless you supply the play-on-load flag, 1 for true). 
Data is either inline xml or a reference to a saved dataset.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		ADD
		(?<template-layer>\d+)
		(?<template>[\d\w]+)
		(?<play-on-load>(0|1))?
		(?<data>".*")
		
Example::

	<<< CG 1-1 ADD 10 svtnews/info 1
	
=========
CG REMOVE
=========
Removes the visible template from a specific layer.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		REMOVE
		(?<template-layer>\d+)
		
Example::

	<<< CG 1-1 REMOVE 1
		
========
CG CLEAR
========
Clears all layers and any state that might be stored. What this actually does behind the scene is to create a new instance of the Adobe Flash player ActiveX controller in memory.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		CLEAR
		
Example::

	<<< CG 1-1 CLEAR

=======
CG PLAY
=======
Plays and displays the template in the specified layer.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		PLAY
		(?<template-layer>\d+)
		
Example::

	<<< CG 1-1 PLAY 1

=======
CG STOP
=======
Stops and removes the template from the specified layer. This is different than REMOVE in that the template gets a chance to animate out when it is stopped.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		STOP
		(?<template-layer>\d+)
		
Example::

	<<< CG 1-1 STOP

=======
CG NEXT
=======
Triggers a "continue" in the template on the specified layer. This is used to control animations that has multiple discreet steps.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		NEXT
		(?<template-layer>\d+)
		
Example::

	<<< CG 1-1 NEXT 1

=======
CG GOTO
=======
Jumps to the specified label in the template on the specified layer.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		GOTO
		(?<template-layer>\d+)
		(?<label>[\d\w]+)
		
Example::

	<<< CG 1-1 GOTO 1 intro
	
=========
CG UPDATE
=========
Sends new data to the template on specified layer. Data is either inline xml or a reference to a saved dataset.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		UPDATE
		(?<template-layer>\d+)
		(?<data>".*")
		
Example::

	<<< CG 1-1 UPDATE 1 "Some data"
	
=========
CG INVOKE
=========
Calls a custom method in the document class of the template on the specified layer. The method must return void and take no parameters.

Syntax::

	CG
		(?<video-channel>[1-9]+)(-(?<layer>\d+))?
		PLAY
		(?<template-layer>\d+)
		(?<method>[\d\w]+)
		
Example::

	<<< CG 1-1 GOTO 1 start_intro
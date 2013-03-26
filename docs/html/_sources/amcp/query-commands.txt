**************
Query Commands
**************

====
CINF
====
Returns information about a mediafile.

Syntax::

	CINF [filename:string]
		
Example::

	>> CINF movie
	<< ...

===
CLS
===
Lists all media files.

Syntax::

	CLS
		
Example::

	>> CLS
	<< ...
	
===
TLS
===
Lists all template files.

Syntax::

	TLS
		
Example::

	>> TLS
	<< ...
	
=======
VERSION
=======
Returns the version of specified component.

Syntax::

	VERSION	SERVER
    VERSION FLASH
    VERSION TEMPLATEHOST
		
Example::

	>> VERSION
	<< ...
	>> VERSION FLASH
	<< ...
	
====
INFO
====
Returns xml-formatted information about the server.

INFO TEMPLATE:  Reads meta-data from a flash-template.
INFO PATHS:     Returns configured paths.
INFO SYSTEM:    Returns information about the system.
INFO CONFIG:    Return the configuration.
INFO:           Returns a list of channels (not xml-formatted due to compatibility issues with older clients).
INFO 1:         Returns information about specified channl.
INFO 1-1:       Returns information about specified layer.
CG 1 INFO       Returns information about flash-producer running on specified channel.


Syntax::

    INFO TEMPLATE [filename:string]
    INFO PATHS
    INFO SYSTEM
    INFO CONFIG
    INFO 
    INFO [channel:int]
    INFO [channel:int]-[layer:int]
    CG [channel:int] INFO
		
Example::

	>> INFO
	<< ...
	>> INFO 1
	<< ...
	>> INFO 1-1
	<< ...
	>> INFO TEMPLATE my_table_template
	<< ...

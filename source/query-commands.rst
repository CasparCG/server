**************
Query Commands
**************

====
CINF
====
Returns information about a mediafile.

Syntax::

	CINF
		(?<filename>[\d\w]+)
		
Example::

	<<< CINF movie
	>>> ...

===
CLS
===
Lists all media files.

Syntax::

	CLS
		
Example::

	<<< CLS
	>>> ...
	
===
TLS
===
Lists all template files.

Syntax::

	TLS
		
Example::

	<<< TLS
	>>> ...
	
=======
VERSION
=======
Returns the version of specified component.

Syntax::

	VERSION
		(FLASH)?
		
Example::

	<<< VERSION
	>>> ...
	<<< VERSION FLASH
	>>> ...
	
====
INFO
====
Returns information about the server. The TEMPLATE version will read and return the meta-data of the specified template.

Syntax::

	VERSION
		((?<video-channel>\d+)(-(?<layer>\d+)))|(TEMPLATE (?<template>[\d\w]+)))?
		
Example::

	<<< INFO
	>>> ...
	<<< INFO 1
	>>> ...
	<<< INFO 1-1
	>>> ...
	<<< INFO TEMPLATE my_table_template
	>>> ...

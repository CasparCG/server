*************
Data Commands
*************
The DATA Commands are convenient to use when you have large datasets that might not be available at broadcast-time. 
DATA allows you to store a dataset on the CasparCG Server and assign it to a much shorter name. This name can then be used to recall the data when displaying a template graphic.

==========
DATA STORE
==========
Stores the dataset data under the name name.

Syntax::

	DATA STORE [name:string] [data:string]
		
Example::

	<<< DATA STORE my_data "Some useful data"
	
=============
DATA RETRIEVE
=============
Returns the data saved under the name *name*.

Syntax::

	DATA RETRIEVE [name:string]
		
Example::

	<<< DATA RETRIEVE my_data
	>>> "Some usefule data"
	
=========
DATA LIST
=========

Syntax::

	DATA LIST

Example::

	<<< DATA LIST
	>>> my_data
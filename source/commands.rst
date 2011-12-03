########
Commands
########

The Advanced Media Control Protocol (AMCP) is the main communication protocol used to control and query CasparCG Server 2.0.

* All communication is presumed to be encoded in UTF-8.
* Each command has to be terminated with both a carriage return and a linefeed character. For example:
	* \r\n
	* <CR><LF>
	* <0x0D><0x0A>
	* <13><10>
* The whole command string is case insensitive.
* Since the parameters in a command is separated by spaces, you need to enclose the parameter with quotation marks if you want it to contain spaces.

***********************
Backwards Compatibility
***********************

The AMCP 2.0 protocol implementation is mostly backward compatible with the previous CasparCG 1.7.1 AMCP Protocol and CasparCG 1.8.0 AMCP Protocol. This is achieved by providing default values for parameters used by the AMCP 2.0 protocol.

================
Breaking Changes
================

* The ''CLEAR'' command will also clear any visible template graphic in the specified container.

*****************
Special sequences
*****************

Since bare quotation marks are used to keep parameters with spaces in one piece, there has to be another way to indicate a quotation mark in a string. Enter special sequences. They behave as in most programming languages. The escape character is the backslash \ character. In order to get a quotation mark you enter \" in the command.
Valid sequences:

* \\" Quotation mark
* \\\\ Backslash
* \\n New line	

These sequences apply to all parameters, it doesn\'t matter if it\'s a file name or a long string of xml-data.

*********************
Command Specification
*********************

Commands are documented as regular expressions. Line-breaks represent whitespace (\s).

************
Return codes
************

===========
Information
===========
* 100 [action] - Information about an event.
* 101 [action] - Information about an event. A line of data is being returned.

==========
Successful
==========
* 200 [command] OK	 - The command has been executed
* 201 [command] OK	 - The command has been executed and a line of data is being returned
* 202 [command] OK	 - The command has been executed and several lines of data are being returned (terminated by an empty line).

============
Client Error
============
* 400 ERROR	 - Command not understood
* 401 [command] ERROR	 - Illegal video_channel
* 402 [command] ERROR	 - Parameter missing
* 403 [command] ERROR	 - Illegal parameter
* 404 [command] ERROR	 - Media file not found

============
Server Error
============
* 500 FAILED	 - Internal server error
* 501 [command] FAILED	- Internal server error
* 502 [command] FAILED	- Media file unreadable

.. include:: basic-commands.rst
.. include:: template-graphics-commands.rst
.. include:: data-commands.rst
.. include:: mixer-commands.rst
.. include:: query-commands.rst
.. include:: misc-commands.rst
	
	
	
	
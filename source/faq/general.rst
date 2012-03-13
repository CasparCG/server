====================
General CasparCG FAQ
====================

Where can I find the documentation for the configuration file (casparcg.config)?
--------------------------------------------------------------------------------

All available options are defined in the commented <!-- --> section found at the bottom of the file.

When I start CasparCG all I get is an empty console window?
-----------------------------------------------------------

You probably have an AMD/ATI graphics card which are not fully compatible with CasparCG 2.0.

NOTE: The currently "unstable" CasparCG 2.1 has better AMD/ATI support and also a cpu fallback.

How can I get fill and key to two separate outputs? e.g. two screens.
---------------------------------------------------------------------

It is possible to only output key to an output using the "key-only" option.

::

    <consumer>
        <screen>
            <device>1</device>
        </screen>
        <screen>
            <device>2</device>
            <key-only>true</key-only>
        </screen>
    </consumers>
    
What kind of transition animations are supported?
-------------------------------------------------

CasparCG can do PUSH, WIPE, SLIDE and MIX using the LOAD, LOADBG and PLAY commands.

It is also possible to create more advanced custom transitions using MIXER commands.
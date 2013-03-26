==================
Decklink FAQ
==================

I can't get both input and output to work on my Decklink card?
--------------------------------------------------------------

Decklink devices are not duplex and only supports either input or output, not both.

How can I get external key using Decklink Quad?
-----------------------------------------------

Decklink Quad does not have native support for external-key. However, you can configure CasparCG
to use one of the four available devices as a key output using the "key-only" option.

::

    <consumer>
        <decklink>
            <device>1</device>
        </decklink>
        <decklink>
            <device>2</device>
            <key-only>true</key-only>
        </decklink>
    </consumers>
    
What Decklink cards support external and internal key?
-------------------------------------------------------

+----------------------+----------+----------+------+
| Device               | Internal | External |  HD  |
+======================+==========+==========+======+
| Intensity Pro        |    no    |    no    |  no  |
+----------------------+----------+----------+------+
| Intensity Shuttle    |    yes   |    no    |  no  |
+----------------------+----------+----------+------+
| Decklink SDI         |    yes   |    no    |  no  |
+----------------------+----------+----------+------+
| Decklink Duo         |    yes   |    no    |  no  |
+----------------------+----------+----------+------+
| Decklink Studio      |    yes   |    yes   |  no  |
+----------------------+----------+----------+------+
| Ultra Studio Pro     |    yes   |    yes   |  no  |
+----------------------+----------+----------+------+
| Decklink HD Extreme  |    yes   |    yes   |  yes |
+----------------------+----------+----------+------+
| Multibridge Pro      |    yes   |    yes   |  yes |
+----------------------+----------+----------+------+
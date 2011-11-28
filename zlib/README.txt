
What's here
===========
  The official ZLIB1.DLL


Source
======
  zlib version 1.2.5
  available at http://www.gzip.org/zlib/


Specification and rationale
===========================
  See the accompanying DLL_FAQ.txt


Usage
=====
  See the accompanying USAGE.txt


Build info
==========
  Contributed by Cosmin Truta.
  Import lib file (zdll.lib) contributed by Gilles Vollant.

  Compiler:
    gcc-4.5.0-1-mingw32
  Library:
    mingwrt-3.17, w32api-3.14
  Build commands:
    gcc -c -DASMV contrib/asm686/match.S
    gcc -c -DASMINF -I. -O3 contrib/inflate86/inffas86.c
    make -f win32/Makefile.gcc LOC="-DASMV -DASMINF" OBJA="inffas86.o match.o"


Changes from zlib125-dll.zip
============================
   Built zdll.lib using Microsoft Visual Studio 2010


Copyright notice
================
  Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu


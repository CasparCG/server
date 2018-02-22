@echo off
REM
REM Copyright 2005-2011 Intel Corporation.  All Rights Reserved.
REM
REM This file is part of Threading Building Blocks.
REM
REM Threading Building Blocks is free software; you can redistribute it
REM and/or modify it under the terms of the GNU General Public License
REM version 2 as published by the Free Software Foundation.
REM
REM Threading Building Blocks is distributed in the hope that it will be
REM useful, but WITHOUT ANY WARRANTY; without even the implied warranty
REM of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with Threading Building Blocks; if not, write to the Free Software
REM Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
REM
REM As a special exception, you may use this file as part of a free software
REM library without restriction.  Specifically, if other files instantiate
REM templates or use macros or inline functions from this file, or you compile
REM this file and link it with other files to produce an executable, this
REM file does not by itself cause the resulting executable to be covered by
REM the GNU General Public License.  This exception does not however
REM invalidate any other reasons why the executable file might be covered by
REM the GNU General Public License.
REM
SET TBB30_INSTALL_DIR=SUBSTITUTE_INSTALL_DIR_HERE
SET TBB_ARCH_PLATFORM=ia32\vc10
SET PATH=%TBB30_INSTALL_DIR%\bin\%TBB_ARCH_PLATFORM%;%PATH%
SET LIB=%TBB30_INSTALL_DIR%\lib\%TBB_ARCH_PLATFORM%;%LIB%
SET INCLUDE=%TBB30_INSTALL_DIR%\include;%INCLUDE%

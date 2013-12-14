@echo off
REM
REM Copyright 2005-2013 Intel Corporation.  All Rights Reserved.
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

set SCRIPT_NAME=%~nx0
if (%1) == () goto Syntax

SET TBB_BIN_DIR=%~d0%~p0

SET TBBROOT=%TBB_BIN_DIR%..

:ParseArgs
:: Parse the incoming arguments
if /i "%1"==""        goto SetEnv
if /i "%1"=="ia32"         (set TBB_TARGET_ARCH=ia32)    & shift & goto ParseArgs
if /i "%1"=="intel64"      (set TBB_TARGET_ARCH=intel64) & shift & goto ParseArgs
if /i "%1"=="vs2005"       (set TBB_TARGET_VS=vc8)       & shift & goto ParseArgs
if /i "%1"=="vs2008"       (set TBB_TARGET_VS=vc9)       & shift & goto ParseArgs
if /i "%1"=="vs2010"       (set TBB_TARGET_VS=vc10)      & shift & goto ParseArgs
if /i "%1"=="vs2012"       (set TBB_TARGET_VS=vc11)      & shift & goto ParseArgs
if /i "%1"=="all"          (set TBB_TARGET_VS=vc_mt)     & shift & goto ParseArgs

:SetEnv

if  ("%TBB_TARGET_VS%") == ("") set TBB_TARGET_VS=vc_mt

SET TBB_ARCH_PLATFORM=%TBB_TARGET_ARCH%\%TBB_TARGET_VS%
if exist "%TBB_BIN_DIR%\%TBB_ARCH_PLATFORM%\tbb.dll" SET PATH=%TBB_BIN_DIR%\%TBB_ARCH_PLATFORM%;%PATH%
if exist "%TBBROOT%\..\redist\%TBB_TARGET_ARCH%\tbb\%TBB_TARGET_VS%\tbb.dll" SET PATH=%TBBROOT%\..\redist\%TBB_TARGET_ARCH%\tbb\%TBB_TARGET_VS%;%PATH%
SET LIB=%TBBROOT%\lib\%TBB_ARCH_PLATFORM%;%LIB%
SET INCLUDE=%TBBROOT%\include;%INCLUDE%
IF ("%ICPP_COMPILER11%") NEQ ("") SET TBB_CXX=icl.exe
IF ("%ICPP_COMPILER12%") NEQ ("") SET TBB_CXX=icl.exe
IF ("%ICPP_COMPILER13%") NEQ ("") SET TBB_CXX=icl.exe
goto End

:Syntax
echo Syntax:
echo  %SCRIPT_NAME% ^<arch^> ^<vs^>
echo    ^<arch^> must be is one of the following
echo        ia32         : Set up for IA-32  architecture
echo        intel64      : Set up for Intel(R) 64  architecture
echo    ^<vs^> should be one of the following
echo        vs2005      : Set to use with Microsoft Visual Studio 2005 runtime DLLs
echo        vs2008      : Set to use with Microsoft Visual Studio 2008 runtime DLLs
echo        vs2010      : Set to use with Microsoft Visual Studio 2010 runtime DLLs
echo        vs2012      : Set to use with Microsoft Visual Studio 2012 runtime DLLs
echo        all         : Set to use TBB statically linked with Microsoft Visual C++ runtime
echo    if ^<vs^> is not set TBB statically linked with Microsoft Visual C++ runtime will be used.
exit /B 1

:End
exit /B 0
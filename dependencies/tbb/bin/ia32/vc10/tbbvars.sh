#!/bin/sh
#
# Copyright 2005-2011 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks.
#
# Threading Building Blocks is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# Threading Building Blocks is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Threading Building Blocks; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As a special exception, you may use this file as part of a free software
# library without restriction.  Specifically, if other files instantiate
# templates or use macros or inline functions from this file, or you compile
# this file and link it with other files to produce an executable, this
# file does not by itself cause the resulting executable to be covered by
# the GNU General Public License.  This exception does not however
# invalidate any other reasons why the executable file might be covered by
# the GNU General Public License.

TBB30_INSTALL_DIR="SUBSTITUTE_SH_INSTALL_DIR_HERE"; export TBB30_INSTALL_DIR
TBB_ARCH_PLATFORM="ia32/vc10"
if [ -z "${PATH}" ]; then
    PATH="${TBB30_INSTALL_DIR}/bin/${TBB_ARCH_PLATFORM}"; export PATH
else
    PATH="${TBB30_INSTALL_DIR}/bin/${TBB_ARCH_PLATFORM};$PATH"; export PATH
fi
if [ -z "${LIB}" ]; then
    LIB="${TBB30_INSTALL_DIR}/lib/${TBB_ARCH_PLATFORM}"; export LIB
else
    LIB="${TBB30_INSTALL_DIR}/lib/${TBB_ARCH_PLATFORM};$LIB"; export LIB
fi
if [ -z "${INCLUDE}" ]; then
    INCLUDE="${TBB30_INSTALL_DIR}/include"; export INCLUDE
else
    INCLUDE="${TBB30_INSTALL_DIR}/include;$INCLUDE"; export INCLUDE
fi

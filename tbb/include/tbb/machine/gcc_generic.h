/*
    Copyright 2005-2011 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_machine_H
#error Do not include this file directly; include tbb_machine.h instead
#endif

#include <stdint.h>
#include <unistd.h>

#define __TBB_WORDSIZE      __SIZEOF_INT__

//for some unknown reason straight mapping does not work. At least on mingw
#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
    #define __TBB_BIG_ENDIAN    0
#elif __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
    #define __TBB_BIG_ENDIAN    1
#else
#error "This endiannes is not supported."
#endif

//As the port has absolutely no information about underlying hardware, the performance,
//most likely, will be sub-optimal, due to usage of full memory fence where a lightweight
//one would suffice..
#define __TBB_acquire_consistency_helper()  __sync_synchronize()
#define __TBB_release_consistency_helper()  __sync_synchronize()
#define __TBB_full_memory_fence()           __sync_synchronize()
#define __TBB_control_consistency_helper()  __sync_synchronize()


#define __MACHINE_DECL_ATOMICS(S,T)                                                               \
inline T __TBB_generic_gcc_cmpswp##S(volatile void *ptr, T value, T comparand ) {                 \
    return __sync_val_compare_and_swap(reinterpret_cast<volatile T *>(ptr),comparand,value);      \
}                                                                                                 \

__MACHINE_DECL_ATOMICS(1,int8_t)
__MACHINE_DECL_ATOMICS(2,int16_t)
__MACHINE_DECL_ATOMICS(4,int32_t)
__MACHINE_DECL_ATOMICS(8,int64_t)

#define __TBB_CompareAndSwap1(P,V,C) __TBB_generic_gcc_cmpswp1(P,V,C)
#define __TBB_CompareAndSwap2(P,V,C) __TBB_generic_gcc_cmpswp2(P,V,C)
#define __TBB_CompareAndSwap4(P,V,C) __TBB_generic_gcc_cmpswp4(P,V,C)
#define __TBB_CompareAndSwap8(P,V,C) __TBB_generic_gcc_cmpswp8(P,V,C)

#if (__TBB_WORDSIZE==4)
    #define __TBB_CompareAndSwapW(P,V,C) __TBB_CompareAndSwap4(P,V,C)
#elif  (__TBB_WORDSIZE==8)
    #define __TBB_CompareAndSwapW(P,V,C) __TBB_CompareAndSwap8(P,V,C)
#else
    #error "Unsupported word size."
#endif

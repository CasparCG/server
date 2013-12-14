/*  Berkelium Utilities -- Berkelium Utilities
 *  Singleton.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Berkelium nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BERKELIUM_SINGLETON_HPP_
#define _BERKELIUM_SINGLETON_HPP_
#include <memory>
namespace Berkelium {

template <class T> class AutoSingleton {
    static std::auto_ptr<T>sInstance;
public:
    static T& getSingleton() {
        if (sInstance.get()==NULL)  {
            throw std::exception();
        }
        return *static_cast<T*>(sInstance.get());
    }
    AutoSingleton() {
        if (sInstance.get()==NULL) {
            std::auto_ptr<T> tmp(static_cast<T*>(this));
            sInstance=tmp;
        }
    }
    virtual ~AutoSingleton() {
        if (sInstance.get()==this)
            sInstance.release();
    }
    static void destroy() {
        sInstance.reset();
    }
};

}
#ifdef _WIN32
#define AUTO_SINGLETON_INSTANCE(ClassName) template<>std::auto_ptr<ClassName>Berkelium::AutoSingleton<ClassName>::sInstance
#else
#define AUTO_SINGLETON_INSTANCE(ClassName) template std::auto_ptr<ClassName> Berkelium::AutoSingleton<ClassName>::sInstance; template<>std::auto_ptr<ClassName>Berkelium::AutoSingleton<ClassName>::sInstance
#endif

#endif

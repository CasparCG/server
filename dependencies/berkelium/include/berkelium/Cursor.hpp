/*  Berkelium - Embedded Chromium
 *  Cursor.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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
 *  * Neither the name of Sirikata nor the names of its contributors may
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

#ifndef _BERKELIUM_CURSOR_HPP_
#define _BERKELIUM_CURSOR_HPP_

#include "berkelium/Platform.hpp"

#if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
#include <windows.h>
#elif BERKELIUM_PLATFORM == PLATFORM_LINUX
// FIXME this really only works for toolkit == gtk
// We use alternate definitions since we can't properly forward
// declare these types
typedef int GdkCursorEnum;
#define GDK_CURSOR_TYPE_FROM_ENUM(X) ((GdkCursorType)X)
typedef void* GdkCursorPtr;
#define GDK_CURSOR_FROM_PTR(X) ((GdkCursor*)X)
#elif BERKELIUM_PLATFORM == PLATFORM_MAC
#ifdef __OBJC__
@class NSCursor;
#else
class NSCursor;
#endif
#endif

namespace Berkelium {

class WindowImpl;

/** A sort-of cross platform cursor class. Currently this is just a
 *  thinner version of Chromium's WebCursor.  Ideally we could figure
 *  a way to hide these details and the application could perform any
 *  necessary translation.
 */
class BERKELIUM_EXPORT Cursor {
public:
#if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
  HCURSOR GetCursor() const;
#elif BERKELIUM_PLATFORM == PLATFORM_LINUX
  GdkCursorEnum GetCursorType() const;
  GdkCursorPtr GetCustomCursor() const;
#elif BERKELIUM_PLATFORM == PLATFORM_MAC
  NSCursor* GetCursor() const;
#endif

private:
    friend class WindowImpl;

    Cursor(); // Non-copyable

#if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
    Cursor(const HCURSOR handle);

    HCURSOR mHandle;
#elif BERKELIUM_PLATFORM == PLATFORM_LINUX
    Cursor(const GdkCursorEnum& _type, GdkCursorPtr _cursor);

    GdkCursorEnum mType;
    GdkCursorPtr mCursor;
#elif BERKELIUM_PLATFORM == PLATFORM_MAC
    Cursor(NSCursor* _cursor);

    NSCursor* mCursor;
#endif
};

} // namespace Berkelium

#endif //_BERKELIUM_CURSOR_HPP_

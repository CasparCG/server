/*  Berkelium - Embedded Chromium
 *  StringUtil.hpp
 *
 *  Copyright (c) 2010, Patrick Reiter Horn
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

#ifndef _BERKELIUM_STRING_URIL_HPP_
#define _BERKELIUM_STRING_URIL_HPP_

#include "berkelium/WeakString.hpp"

namespace Berkelium {

typedef unsigned short char16;

typedef WeakString<char> UTF8String;
typedef WeakString<char16> UTF16String;

WideString BERKELIUM_EXPORT UTF8ToWide(const UTF8String &in);
UTF8String BERKELIUM_EXPORT WideToUTF8(const WideString &in);

WideString BERKELIUM_EXPORT UTF16ToWide(const UTF16String &in);
UTF16String BERKELIUM_EXPORT WideToUTF16(const WideString &in);

UTF8String BERKELIUM_EXPORT UTF16ToUTF8(const UTF16String &in);
UTF16String BERKELIUM_EXPORT UTF8ToUTF16(const UTF8String &in);

void BERKELIUM_EXPORT stringUtil_free(WideString returnedValue);
void BERKELIUM_EXPORT stringUtil_free(UTF8String returnedValue);
void BERKELIUM_EXPORT stringUtil_free(UTF16String returnedValue);

}

#endif


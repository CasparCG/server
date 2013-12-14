/*  Berkelium - Embedded Chromium
 *  SafeString.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#ifndef _BERKELIUM_WEAK_STRING_HPP_
#define _BERKELIUM_WEAK_STRING_HPP_

#include "berkelium/Platform.hpp"

namespace Berkelium {

// Simple POD string class. Data must be owned by caller.
template <class CharType>
struct WeakString {
    const CharType* mData;
    size_t mLength;

    typedef CharType Type;

    inline const CharType* data() const {
        return mData;
    }

    inline size_t length() const {
        return mLength;
    }

    inline size_t size() const {
        return mLength;
    }

    template <class StrType>
    inline StrType& get(StrType& ret) const {
        if (!mData || !mLength) {
            ret = StrType();
        }
        ret = StrType(mData, mLength);
        return ret;
    }

    template <class StrType>
    inline StrType get() const {
        if (!mData || !mLength) {
            return StrType();
        }
        return StrType(mData, mLength);
    }

    template <class StrType>
    inline static WeakString<CharType> point_to(const StrType&input) {
        WeakString<CharType> ret;
        ret.mData = input.data();
        ret.mLength = input.length();
        return ret;
    }

    inline static WeakString<CharType> point_to(const CharType*input_data,
                                                size_t input_length) {
        WeakString<CharType> ret;
        ret.mData = input_data;
        ret.mLength = input_length;
        return ret;
    }

    inline static WeakString<CharType> point_to(const CharType *input_data) {
        WeakString<CharType> ret;
        ret.mData = input_data;
        for (ret.mLength = 0; input_data[ret.mLength]; ++ret.mLength) {
        }
        return ret;
    }

    inline static WeakString<CharType> empty() {
        WeakString<CharType> ret;
        ret.mData = NULL;
        ret.mLength = 0;
        return ret;
    }
};

template <class StrType, class CharType>
inline StrType &operator+(const StrType&lhs, const WeakString<CharType>&rhs) {
    StrType temp;
    return lhs + rhs.get(temp);
}

template <class StrType, class CharType>
inline StrType &operator+=(StrType&lhs, const WeakString<CharType>&rhs) {
    StrType temp;
    return lhs += rhs.get(temp);
}

template <class OstreamType, class CharType>
inline OstreamType &operator<< (OstreamType&lhs, const WeakString<CharType>&rhs) {
    size_t length = rhs.length();
    const CharType *data = rhs.data();
    for (size_t i = 0; i < length; i++) {
        lhs << data[i];
    }
    return lhs;
}

typedef WeakString<char> URLString;
typedef WeakString<wchar_t> WideString;

#if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
typedef WeakString<wchar_t> FileString;
#else
typedef WeakString<char> FileString;
#endif

}

#endif

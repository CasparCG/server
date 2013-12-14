/*  Berkelium - Embedded Chromium
 *  Rect.hpp
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

#ifndef _BERKELIUM_RECT_HPP_
#define _BERKELIUM_RECT_HPP_

namespace Berkelium {

struct Rect {
    int mLeft;
    int mTop;
    int mWidth;
    int mHeight;

    inline int y() const { return mTop; }
    inline int x() const { return mLeft; }
    inline int top() const { return mTop; }
    inline int left() const { return mLeft; }
    inline int width() const { return mWidth; }
    inline int height() const { return mHeight; }
    inline int right() const { return mLeft + mWidth; }
    inline int bottom() const { return mTop + mHeight; }

    template <class T>
    inline void setFromRect(const T&sourceRect) {
        mLeft = sourceRect.x();
        mTop = sourceRect.y();
        mWidth = sourceRect.width();
        mHeight = sourceRect.height();
    }

    inline bool contains(int x, int y) const {
        return (x >= left() && x < right() &&
                y >= top() && y < bottom());
    }
    Rect intersect(const Rect &rect) const {
        int rx = rectmax(left(), rect.left());
        int ry = rectmax(top(), rect.top());
        int rr = rectmin(right(), rect.right());
        int rb = rectmin(bottom(), rect.bottom());
        if (rx >= rr || ry >= rb)
            rx = ry = rr = rb = 0;  // non-intersecting
        Rect ret;
        ret.mLeft = rx;
        ret.mTop = ry;
        ret.mWidth = rr-rx;
        ret.mHeight = rb-ry;
        return ret;
    }

    /** Create a new Rect equivalent to this Rect translated by the specified
     *  amounts in either direction.
     *  \param dx amount to translate along the X axis
     *  \param dy amount to translate along the Y axis
     *  \returns a new translated Rect
     */
    Rect translate(int dx, int dy) const {
        Rect ret = *this;
        ret.mLeft += dx;
        ret.mTop += dy;
        return ret;
    }

private:
    static int rectmax(int a, int b) {
        return a>b?a:b;
    }
    static int rectmin(int a, int b) {
        return a<b?a:b;
    }
};

}

#endif

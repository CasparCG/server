/*  Berkelium - Embedded Chromium
 *  Widget.hpp
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

#ifndef _BERKELIUM_WIDGET_HPP_
#define _BERKELIUM_WIDGET_HPP_

#include "berkelium/Rect.hpp"
#include "berkelium/WeakString.hpp"

namespace Berkelium {

/** A widget is a rectangular canvas which can be painted to. A Widget
 *  maintains its own focus state, and can accept input just like a Window.
 *  Widgets have very limited use in practice--mostly just dropdowns.
 */
class BERKELIUM_EXPORT Widget {
public:
    /** Deprecated virtual destructor.
     *  \deprecated Use the safer destroy() method instead.
     */
    virtual ~Widget() {}

    /** Safe destructor for widget. Should clean up all resources.
     *  Note: WindowDelegate::onWidgetDestroyed will be called synchronously.
     */
    void destroy(); // defined in src/RenderWidget.cpp

    /** Gets a unique id for this widget.
     *  \returns the widget's routing id
     */
    virtual int getId() const = 0;

    virtual void focus()=0;
    virtual void unfocus()=0;
    virtual bool hasFocus() const = 0;

    virtual void mouseMoved(int xPos, int yPos)=0;
    virtual void mouseButton(unsigned int buttonID, bool down)=0;
    virtual void mouseWheel(int xScroll, int yScroll)=0;

    virtual void textEvent(const wchar_t* evt, size_t evtLength)=0;
    virtual void keyEvent(bool pressed, int mods, int vk_code, int scancode)=0;

    virtual Rect getRect() const=0;
    virtual void setPos(int x, int y)=0;

    virtual void textEvent(WideString text)=0;
};

}

#endif

/*  Berkelium - Embedded Chromium
 *  Window.hpp
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

#ifndef _BERKELIUM_WINDOW_HPP_
#define _BERKELIUM_WINDOW_HPP_

#include <vector>

#include "berkelium/WeakString.hpp"

namespace Berkelium {

class Widget;
class WindowDelegate;
class Context;

namespace Script{
class Variant;
}

enum KeyModifier {
    SHIFT_MOD      = 1 << 0,
    CONTROL_MOD    = 1 << 1,
    ALT_MOD        = 1 << 2,
    META_MOD       = 1 << 3,
    KEYPAD_KEY     = 1 << 4, // If the key is on the keypad (use instead of keypad-specific keycodes)
    AUTOREPEAT_KEY = 1 << 5, // If this is not the first KeyPress event for this key
    SYSTEM_KEY     = 1 << 6 // if the keypress is a system event (WM_SYS* messages in windows)
};

/** Windows are individual web pages, the equivalent of a single tab in a normal
 *  browser.  Windows mediate interaction between the user and the
 *  renderer. They allow inspection of the page (access to UI widgets),
 *  injection of input (keyboard, mouse), injection of Javascript code,
 *  navigation controls (forward, back, loading URLs), and other utility methods
 *  (cut, copy, paste, etc).  Since the represent off-screen renderers, they
 *  also allow manipulation such as resizting.
 */
class BERKELIUM_EXPORT Window {
protected:
    typedef std::vector<Widget*> WidgetList;

public:
    typedef WidgetList::const_iterator BackToFrontIter;
    typedef WidgetList::const_reverse_iterator FrontToBackIter;

protected:
    /** Construct a completely uninitialized Window -- it will have no backing
     *  renderer or delegate and use a new Context.
     */
    Window ();
    /** Construct a Window which uses the specified Context for rendering.
     *  \param otherContext an existing rendering Context to use
     */
    Window (const Context*otherContext);

public:
    /** Create a new Window with all default properties which uses an existing
     *  Context for rendering.  It will be zero sized and use a default local
     *  web page.
     *  \param context an existing context to use for rendering
     *  \returns a new Window or NULL on failure
     */
    static Window* create (const Context * context);

    /** Deletes this window object.
     */
    void destroy();

    /** Deprecated virtual destructor.
     *  \deprecated Use destroy() to avoid interference from custom allocators.
     */
    virtual ~Window();

    /** A Window should have a root widget object, unless the window crashed
     *  or has never been navigated.
     *  If such a widget exists, returns non-null.
     */
    virtual Widget* getWidget() const=0;

    /** Get the rendering context for this Window. */
    inline Context *getContext() const {
        return mContext;
    }

    /** Set the delegate for receiving events, such as paint events and alerts,
     *  from this Window.
     *  \param delegate the WindowDelegate that should receive events, or NULL
     *         to disable delegation of events
     */
    void setDelegate(WindowDelegate *delegate) {
        mDelegate = delegate;
    }

    /** loop from the backmost (usually obscured) widget to the
     *  frontmost (focused) widget.
     */
    BackToFrontIter backIter() const {
        return mWidgets.begin();
    }

    /** Corresponding end() to compare against the backIter. */
    BackToFrontIter backEnd() const {
        return mWidgets.end();
    }

    /** loop from the frontmost (focused) widget to the
     *  backmost (usually obscured) widget.
     */
    FrontToBackIter frontIter() const {
        return mWidgets.rbegin();
    }

    /** Corresponding end() to compare against the frontIter. */
    FrontToBackIter frontEnd() const {
        return mWidgets.rend();
    }

    /** Look up which widget is at the specified point in the Window.
     *  \param xPos the position on the x-axis of the point to look up
     *  \param yPos the position on the y-axis of the point to look up
     *  \param returnRootIfOutside if true and the specified point lies outside
     *         the Window, returns the root Widget instead of NULL
     *  \returns the Widget at the specified location or NULL if no widgets are
     *           at that point
     */
    Widget *getWidgetAtPoint(int xPos, int yPos, bool returnRootIfOutside=false) const;

    /** Gets an id for this window.
     * \returns the RenderView's unique routing id.
     */
    virtual int getId() const = 0;

    /** Sets the transparency flag for this Window. Note that the buffer will
     * be BGRA regardless of transparency, but if the window is not set as
     * transparent, the alpha channel should always be 1.
     * Transparency defaults to false and must be enabled on each Window.
     * \param istrans  Whether to enable a transparent background.
     */
    virtual void setTransparent(bool istrans)=0;

    /** Set the topmost Widget for this Window as focused.
     */
    virtual void focus()=0;

    /** Blurs all widgets on this Window.
     */
    virtual void unfocus()=0;

    /** Inject a mouse movement event into the Window at the specified position.
     *  \param xPos the position along the x-axis of the mouse movement
     *  \param yPos the position along the y-axis of the mouse movement
     */
    virtual void mouseMoved(int xPos, int yPos)=0;
    /** Inject a mouse button event into the Window.
     *  \param buttonID index of the button that caused the event
     *  \param down if true, indicates the mouse button was pressed, if false
     *         indicates it was released
     */
    virtual void mouseButton(unsigned int buttonID, bool down)=0;
    /** Inject a mouse wheel scroll event into the Window.
     *  \param xScroll amount scrolled horizontally
     *  \param yScroll amount scrolled vertically
     */
    virtual void mouseWheel(int xScroll, int yScroll)=0;

    /** Inject a text event into the Window.
     *  \param evt pointer to text string to inject
     *  \param evtLength length of string
     */
    virtual void textEvent(const wchar_t *evt, size_t evtLength)=0;

    /** Inject an individual key event into the Window.
     *  \param pressed if true indicates the key was pressed, if false indicates
     *         it was released
     *  \param mods a modifier code created by a logical or of KeyModifiers
     *  \param vk_code the virtual key code received from the OS
     *  \param scancode the original scancode that generated the event
     */
    virtual void keyEvent(bool pressed, int mods, int vk_code, int scancode)=0;


    /** Resize the Window. You should receive an onPaint message as an
     *  acknowledgement.
     *  \param width the new width
     *  \param height the new height
     */
    virtual void resize(int width, int height)=0;

    /** Changes the zoom level of the page in fixed increments (same as
     *  Ctrl--, Ctrl-0, and Ctrl-+ in most browsers.
     * \param mode  -1 to zoom out, 0 to reset zoom, 1 to zoom in
     */
    virtual void adjustZoom (int mode)=0;

    /** Execute Javascript in the context of the Window. This is equivalent to
     *  executing Javascript in the address bar of a regular browser. The
     *  javascript is copied so the caller retains ownership of the string.
     *  \param javascript pointer to a string containing Javascript code
     */
    virtual void executeJavascript (WideString javascript) = 0;

    /** Insert the given text as a STYLE element at the beginning of the
     * document.
     * \param css  Stylesheet content to insert.
     * \param elementId  Can be empty, but if specified then it is used
     * as the id for the newly inserted element (replacing an existing one
     * with the same id, if any).
     */
    virtual void insertCSS (WideString css, WideString elementId) = 0;

    /** Request navigation to a URL. Depending on the url, this might not
     *  cause an actual navigation.
     *  \param url  URLString pointer to an ASCII URL.
     */
    virtual bool navigateTo(URLString url)=0;

    /**
     * Request navigation to a URL.  The URL string is copied so the caller
     * retains ownership of the string.
     * \deprecated Use navigateTo(URLString) instead
     * \param url pointer to an ASCII string containing a URL
     * \param url_length the length of the URL string
     */
    inline bool navigateTo(const char *url, size_t url_length) {
        return navigateTo(URLString::point_to(url,url_length));
    }

    /** Request that the page be reloaded. */
    virtual void refresh() = 0;

    /** Stop ongoing navigations. */
    virtual void stop() = 0;

    /** Goes back by one history item. */
    virtual void goBack() = 0;

    /** Goes forward by one history item. */
    virtual void goForward() = 0;

    /** True if you can go back at all (if back button should be enabled). */
    virtual bool canGoBack() const = 0;

    /** True if you can go forward (if forwrad button should be enabled). */
    virtual bool canGoForward() const = 0;

    /** Cut the currently selected data to the clipboard. */
    virtual void cut()=0;

    /** Copy the currently selected data to the clipboard. */
    virtual void copy()=0;

    /** Paste the current clipboard contents to the current cursoor position in
     *  the Window.
     */
    virtual void paste()=0;

    /** Request the last action be undone. */
    virtual void undo()=0;

    /** Request the last undone action be reperformed. */
    virtual void redo()=0;

    /** Request deletion action. */
    virtual void del()=0;

    /** Request all data be selected. */
    virtual void selectAll()=0;

    /** Call after the file chooser has finished. Cancels if |files| is NULL or empty.
     * \param files  List of FileString's terminated with empty FileString.
     */
    virtual void filesSelected(FileString *files)=0;

    /** Call after the file chooser has finished. Cancels if |files| is NULL or empty.
     * Note: This *MUST* be called after onJavascriptCallback is called with synchronous.
     * If not called, the owning RenderViewHost will no longer run scripts.
     *
     * \param handle  Opaque |replyMsg| passed in WindowDelegate::onJavascriptCallback
     * \param result  Javascript value to return.
     */
    virtual void synchronousScriptReturn(void *handle, const Script::Variant &result)=0;

    /** Binds a proxy function in Javascript to call */
    virtual void bind(WideString lvalue, const Script::Variant &rvalue)=0;

    /** Binds a proxy function in Javascript at the beginning of each page load */
    virtual void addBindOnStartLoading(WideString lvalue, const Script::Variant &rvalue)=0;

    /** Runs code in Javascript at the beginning of each page load */
    virtual void addEvalOnStartLoading(WideString script)=0;

    /** Removes all bindings in Javascript */
    virtual void clearStartLoading()=0;

protected:
    void appendWidget(Widget *wid) {
        mWidgets.push_back(wid);
    }
    void removeWidget(Widget *wid) {
        for (WidgetList::iterator it = mWidgets.begin();
             it != backEnd();
             ++it)
        {
            if (*it == wid) {
                mWidgets.erase(it);
                return;
            }
        }
    }

protected:
    Context *mContext;
    WindowDelegate *mDelegate;

    WidgetList mWidgets;
};

}

#endif

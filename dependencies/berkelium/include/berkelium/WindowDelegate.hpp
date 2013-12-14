/*  Berkelium - Embedded Chromium
 *  WindowDelegate.hpp
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

#ifndef _BERKELIUM_WINDOW_DELEGATE_HPP_
#define _BERKELIUM_WINDOW_DELEGATE_HPP_

#include "berkelium/WeakString.hpp"
#include "berkelium/Rect.hpp"
#include "berkelium/ScriptVariant.hpp"
#include "berkelium/Window.hpp"

namespace Berkelium {

class WindowImpl;
class Widget;
class Window;
class Cursor;

/**
 * Holds parameters for the onContextMenu delegate method.
 */
struct ContextMenuEventArgs {
  enum MediaType {
      MediaTypeNone,
      MediaTypeImage,
      MediaTypeVideo,
      MediaTypeAudio,
  };
  enum EditFlags {
      CanDoNone = 0x0,
      CanUndo = 0x1,
      CanRedo = 0x2,
      CanCut = 0x4,
      CanCopy = 0x8,
      CanPaste = 0x10,
      CanDelete = 0x20,
      CanSelectAll = 0x40,
  };

  MediaType mediaType;

  int mouseX, mouseY;

  URLString linkUrl, srcUrl, pageUrl, frameUrl;
  WideString selectedText;

  bool isEditable;

  int editFlags;
};

enum ScriptAlertType {
	JavascriptAlert = 0,
	JavascriptConfirm = 1,
	JavascriptPrompt = 2
};

enum FileChooserType {
    FileOpen = 0,
    FileOpenMultiple = 1,
    FileOpenFolder = 2,
    FileSaveAs = 3
};

/**
 * Override WindowDelegate methods to intercept events from a render view.
 */
class BERKELIUM_EXPORT WindowDelegate {
public:
    /**
     * Virtual destructor is not necessary--Berkelium will not delete a
     * WindowDelegate.
     */
    virtual ~WindowDelegate() {}

    /**
     * The URL associated with this page has changed. This is similar to
     * onStartLoading, but may also be called if the hash part of the url
     * changes.
     *
     * \param win  Window instance that fired this event.
     * \param newURL  URL to display or store.
     */
    virtual void onAddressBarChanged(Window *win, URLString newURL) {}
    /**
     * Indicates whether the main page has started navigating to a new URL.
     * \note See onLoadingStateChanged for deciding to show a loading message.
     *
     * \param win  Window instance that fired this event.
     * \param newURL  URL of the currently-loading page for the main frame.
     */
    virtual void onStartLoading(Window *win, URLString newURL) {}
    /**
     * A page has loaded without error.
     *
     * \param win  Window instance that fired this event.
     */
    virtual void onLoad(Window *win) {}
    /**
     * A worker has crashed. No info is provided yet to the callback.
     *
     * \param win  Window instance that fired this event.
     */
    virtual void onCrashedWorker(Window *win) {}
    /**
     * Plugin with a given name has crashed. There's currently no way to
     * restart it, aside from maybe reloading the page that uses it.
     *
     * \param win  Window instance that fired this event.
     * \param pluginName  Name reported by the plugin..
     */
    virtual void onCrashedPlugin(Window *win, WideString pluginName) {}
    /**
     * Encountered an error while loading a page, and might not finish loading.
     *
     * \param win  Window instance that fired this event.
     * \param url  URL that encountered the error.
     * \param errorCode  Some enumeration? errno? net error code?
     * \param isMainFrame  True if the error happened for the main page,
     *     If false, it may have happened in an iframe (or XHR?).
     */
    virtual void onProvisionalLoadError(Window *win, URLString url,
                                        int errorCode, bool isMainFrame) {}
    /**
     * Display a javascript message (Maybe an error?) or console.log.
     *
     * \param win  Window instance that fired this event.
     * \param message  String contents of the log message.
     * \param sourceId  Javascript file or other source sending the message.
     * \param line_no  Line number, useful for debugging.
     */
    virtual void onConsoleMessage(Window *win, WideString message,
                                  WideString sourceId, int line_no) {}
    /**
     * Synchronously display an alert box (OK), confirm (OK, CANCEL), or a
     * prompt (Input). If prompt, optionally return a string in value.
     *
     * \param win  Window instance that fired this event.
     * \param message  Alert message
     * \param defaultValue  What to display in a text field if this is prompt.
     * \param url  Originator (script? or page?) of the alert box.
     * \param flags  What type of alert/confirm/prompt. See ScriptAlertType.
     * \param success  True if the OK button, false if CANCEL.
     * \param value  Allocated (strdup is easiest) return, default empty/null.
     */
    virtual void onScriptAlert(Window *win, WideString message,
                              WideString defaultValue, URLString url,
                              int flags, bool &success, WideString &value) {}
    /**
     * Frees resources from the most recent onScriptAlert call, if necessary.
     * Guaranteed to be called within the span of the same update() call, so
     * the WindowDelegate can safely use a variable to hold the string instance
     * while waiting to free it.
     *
     * \param lastValue  A value to pass to the appropriate free method.
     */
    virtual void freeLastScriptAlert(WideString lastValue) {}
    /**
     * A navigation has been requested but not yet started.
     * I'm not sure yet if this gets called for frames.
     *
     * \param win  Window instance that fired this event.
     * \param newUrl  URL which wants to load.
     * \param referrer  Page which initiated the request.
     * \param isNewWindow  The request will result in a popup if not cancelled.
           If true and not cancelled, expect a onCreatedWindow callback soon.
     * \param cancelDefaultAction  Put true into this value to cancel.
     *     For example, if you don't want a new window, call win->navigate.
     */
    virtual void onNavigationRequested(Window *win, URLString newUrl,
                                       URLString referrer, bool isNewWindow,
                                       bool &cancelDefaultAction) {
        // Override to permit popup windows only if you support onCreatedWindow.
        if (isNewWindow) cancelDefaultAction = true;
    }
    /**
     * Notifies if a page is loading.
     * This callback is only useful for deciding whether to show a loading
     * indicator, for example a spinning widget or maybe a capital letter
     * being bombarded by meteors.
     * \note This may happen at different times from onStartLoading.
     *
     * \param win  Window instance that fired this event.
     * \param isLoading  true if loading has started, false if stopped.
     */
    virtual void onLoadingStateChanged(Window *win, bool isLoading) {}
    /**
     * Window's title has changed. This is usually the contents of title,
     * however some pages may dynamically change document.title.
     * Additionally, may be a default title (e.g. a URL) if the page has none.
     *
     * \param win  Window instance that fired this event.
     * \param title  Page title, should be the same as document.title.
     */
    virtual void onTitleChanged(Window *win, WideString title) {}
    /**
     * Display a tooltip at the mouse cursor position.
     * There's no notification for destroying the tooltip afaik
     *
     * \param win  Window instance that fired this event.
     * \param text  Text on the tooltip.
     */
    virtual void onTooltipChanged(Window *win, WideString text) {}

    /**
     * A renderer instance crashed. Applications can't currently do anything
     * to handle this aside from recreating the Window object.
     *
     * \param win  Window instance that fired this event.
     */
    virtual void onCrashed(Window *win) {}
    /**
     * A renderer instance is hung. You can use this to display the Window in
     * a greyed out state, and offer the user a choice to kill the Window.
     *
     * \param win  Window instance that fired this event.
     */
    virtual void onUnresponsive(Window *win) {}
    /**
     * Renderer instance is back to normal. Reset the color and close dialogs.
     *
     * \param win  Window instance that fired this event.
     */
    virtual void onResponsive(Window *win) {}

    /**
     * Javascript has sent an externalHost message to this WindowDelegate.
     * To send a message from javascript call, for example: <code>
     *     window.externalHost.postMessage("message", "urn:uuid:0000-...")
     * </code>
     * \param win  Window instance that fired this event.
     * \param message  Full contents of message sent by javascript.
     * \param origin  Origin header of page sending the request. Use this
     *     if you want to restrict messages to trusted pages.
     * \param target  A URI Specified by the page. Can be in a custum or urn
     *     format if that is useful.
     */
    virtual void onExternalHost(
        Window *win,
        WideString message,
        URLString origin,
        URLString target) {}

    /**
     * A new window has already been created. You are now expected to hold
     * onto the instance and be responsible for deleting it.
     *
     * \param win  Window instance that fired this event.
     * \param newWindow  New Berkelium window object to keep track of.
     *     You may optionally destroy the window if you don't want it.
     * \param initialRect  Coordinates and size that the window is requesting.
     *     Use them as a recommendataion, but it's fine to call resize(),
     *     and x/y coords should be ignored if doing off-screen rendering.
     */
    virtual void onCreatedWindow(Window *win, Window *newWindow,
                                 const Rect &initialRect) {}

    /**
     * The window is being painted. You need to synchronously copy the buffer
     * into application (video) memory before returning.
     *
     * \param win  Window instance that fired this event.
     * \param sourceBuffer  BGRA buffer with width/height of sourceBufferRect.
     * \param sourceBufferRect  Rect containing the buffer.
     * \param numCopyRects  Length of copyRects.
     * \param copyRects  Array of valid+changed rectangles of sourceBuffer.
     *     Anything not in copyRects is usually garbage data.
     *     While these must lie within sourceBufferRect to make sense,
     *     they are not relative to sourceBufferRect.
     * \param dx  If non-zero, the area of the page specified by scrollRect has
     *     been scrolled horizontally.
     * \param dy  If non-zero, the area of the page specified by scrollRect has
     *     been scrolled vertically.
     * \param scrollRect  Area of the page to scroll. Only valid if dx&&dy.
     */
    virtual void onPaint(
        Window *win,
        const unsigned char *sourceBuffer,
        const Rect &sourceBufferRect,
        size_t numCopyRects,
        const Rect *copyRects,
        int dx, int dy,
        const Rect &scrollRect) {}

    /**
     * A widget is a rectangle to display on top of the page, e.g. a context
     * menu or a dropdown.
     *
     * \param win  Window which is gaining a new widget object
     * \param newWidget  Widget object to optionally hold onto.
     * \param zIndex  Sorting value to compare to other Widget's.
     */
    virtual void onWidgetCreated(Window *win, Widget *newWidget, int zIndex) {}
    /**
     * Called by Widget::destroy(), in its destructor. This widget will be
     * removed from the Window's list of widgets when this function returns.
     *
     * \param win  Window which is losing this widget object.
     * \param wid  Widget this event is for.
     */
    virtual void onWidgetDestroyed(Window *win, Widget *wid) {}

    /**
     * Widget has changed size, Usually only happens once after creating.
     *
     * \param win  Window which is losing this widget object.
     * \param wid  Widget this event is for.
     * \param newWidth  Width of widget texture.
     * \param newHeight  Height of widget texture.
     */
    virtual void onWidgetResize(
        Window *win,
        Widget *wid,
        int newWidth,
        int newHeight) {}

    /**
     * Widget has moved, Usually only happens once after creating.
     * While these coordinates may be ignored, the position usually corresponds
     * to some click event or the mouse coordinates.
     *
     * \param win  Window which is losing this widget object.
     * \param wid  Widget this event is for.
     * \param newX  Left coordinate relative to Window.
     * \param newY  Top coordinate relative to Window.
     */
    virtual void onWidgetMove(
        Window *win,
        Widget *wid,
        int newX,
        int newY) {}

    /**
     * A widget overlay has been painted.
     *
     * \see onPaint
     * \param win  Window instance that fired this event.
     * \param wid Widget this event is for
     * \param sourceBuffer  raw buffer.
     * \param sourceBufferRect  Rect containing the buffer.
     * \param numCopyRects  Length of copyRects.
     * \param copyRects  Array of valid+changed rectangles of sourceBuffer.
     * \param dx  delta-x scroll
     * \param dy  delta-y scroll
     * \param scrollRect  Area of the page to scroll. Only valid if dx&&dy.
     */
    virtual void onWidgetPaint(
        Window *win,
        Widget *wid,
        const unsigned char *sourceBuffer,
        const Rect &sourceBufferRect,
        size_t numCopyRects,
        const Rect *copyRects,
        int dx, int dy,
        const Rect &scrollRect) {}

    /**
     * Invoked when the Window requests that the mouse cursor be updated.
     * \param win  Window instance that fired this event.
     * \param newCursor  Platform-specific cursor handle to pass to the
     *     appropriate system API.
     * \see Berkelium::Cursor
     */
    virtual void onCursorUpdated(Window *win, const Cursor& newCursor) {}

    /**
     * Notification that the user has brought up a context menu. Chrome doesn't
     * specify the exact set of context menu items--implementors are free to
     * choose other menu items to display.
     * \param win  Window instance that fired this event.
     * \param args  Guidelines on what menu items to include or not include.
     */
    virtual void onShowContextMenu(Window *win,
                                   const ContextMenuEventArgs& args) {}

    /** Javascript has called a bound function on this Window.
     * Make sure to h
     *
     * \param win  Window instance that fired this event.
     * \param replyMsg  If non-NULL, opaque reply identifier to be passed to synchronousScriptReturn.
     * \param url  Origin of the sending script.
     * \param funcName  name of function to call.
     * \param args  list of variants passed into function.
     * \param numArgs  number of arguments.
     */
    virtual void onJavascriptCallback(Window *win, void* replyMsg, URLString origin, WideString funcName, Script::Variant *args, size_t numArgs) {
        if (replyMsg) {
            win->synchronousScriptReturn(replyMsg, Script::Variant());
        }
    }

    /** Display a file chooser dialog, if necessary. The value to be returned should go ______.
     * \param win  Window instance that fired this event.
     * \param mode  Type of file chooser expected. See FileChooserType.
     * \param title  Title for dialog. "Open" or "Save" should be used if empty.
     * \param defaultFile  Default file to select in dialog.
     */
    virtual void onRunFileChooser(Window *win, int mode, WideString title, FileString defaultFile) {
        win->filesSelected(NULL);
    }

/**************************
   Might want messages for:
- StartDragging(const WebDropData& drop_data,
                WebKit::WebDragOperationsMask allowed_ops);
- UpdateDragCursor(WebKit::WebDragOperation operation);
- TakeFocus(bool reverse)
- Close
- UpdateTargetURL
- UpdateEncoding
**************************/

};

}

#endif

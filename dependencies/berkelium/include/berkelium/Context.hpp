/*  Berkelium - Embedded Chromium
 *  Context.hpp
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

#ifndef _BERKELIUM_CONTEXT_HPP_
#define _BERKELIUM_CONTEXT_HPP_

class SiteInstance;
class BrowsingInstance;

namespace Berkelium {
class ContextImpl;

/** A context holds onto a reference counted profile object.
 *  The Window class calls clone() on a context, so you can safely
 *  destroy a Context after making all the Windows you want.
 *
 *  No functions currently exist for this object.
 */
class BERKELIUM_EXPORT Context {
  protected:
    Context();

public:
    /** Creates an all-new context with no shared state and a refcount of 1.
     */
    static Context* create();

    /** Deletes this Context object, which decrements the refcount for the
     *  underlying context data.
     */
    void destroy();

    /** Deprecated destructor
     *  \deprecated destroy()
     */
    virtual ~Context();

    /** Returns a new Context object which increments the refcount of
     *  the internal context information. Called by Window::create.
     */
    virtual Context* clone() const = 0;


    virtual ContextImpl* getImpl() = 0;
    virtual const ContextImpl* getImpl() const = 0;
};

}

#endif

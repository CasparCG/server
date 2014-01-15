/*  Berkelium - Embedded Chromium
 *  Berkelium.hpp
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

#ifndef _BERKELIUM_HPP_
#define _BERKELIUM_HPP_
#include "berkelium/Platform.hpp"
#include "berkelium/WeakString.hpp"
namespace sandbox {
class BrokerServices;
class TargetServices;
#ifdef _WIN32
enum DepEnforcement;
#endif
}
namespace Berkelium {

/** May be implemented to handle global errors gracefully.
 */
class BERKELIUM_EXPORT ErrorDelegate {
public:
    virtual ~ErrorDelegate() {}
};

/* TODO: Allow forkedProcessHook to be called without requiring the
   library to be initialized/in memory (unless this is a sub-process).
   i.e. an inline function that first searches for "--type=" in argv,
   then uses dlopen or GetProcAddress.
*/

/** Called by child processes.
 * Should only ever be called from subprocess.cpp (berkelium.exe).
 */
#ifdef _WIN32
void BERKELIUM_EXPORT forkedProcessHook(
    sandbox::BrokerServices* (*ptrGetBrokerServices)(),
    sandbox::TargetServices* (*ptrGetTargetServices)(),
    bool (*ptrSetCurrentProcessDEP)(enum sandbox::DepEnforcement));
#else
void BERKELIUM_EXPORT forkedProcessHook(int argc, char **argv);
#endif

/** Iniitialize berkelium's global object.
 *  \param homeDirectory  Just like Chrome's --user-data-dir command line flag.
 *    If homeDirectory is null or empty, creates a temporary data directory.
 *  \param subprocessDirectory  Path to berkelium.exe.
 */
bool BERKELIUM_EXPORT init(FileString homeDirectory, FileString subprocessDirectory);

/** Iniitialize berkelium's global object.
 *  \param homeDirectory  Just like Chrome's --user-data-dir command line flag.
 *    If homeDirectory is null or empty, creates a temporary data directory.
 */
bool BERKELIUM_EXPORT init(FileString homeDirectory);

/** Destroys Berkelium and attempts to free as much memory as possible.
 *  Note: You must destroy all Window and Context objects before calling
 *  Berkelium::destroy()!
 */
void BERKELIUM_EXPORT destroy();

void BERKELIUM_EXPORT setErrorHandler(ErrorDelegate * errorHandler);

/** Runs the message loop until all pending messages are processed.
 *  Must be called from the same thread as all other Berkelium functions,
 *  usually your program's main (UI) thread.
 *  For now, you have to poll to receive updates without blocking indefinitely.
 *
 *  Your WindowDelegate's should only receive callbacks synchronously with
 *  this call to update.
 */
void BERKELIUM_EXPORT update();
void BERKELIUM_EXPORT runUntilStopped();
void BERKELIUM_EXPORT stopRunning();

}

#endif

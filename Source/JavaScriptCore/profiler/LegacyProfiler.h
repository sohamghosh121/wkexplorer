/*
 * Copyright (C) 2008, 2012, 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LegacyProfiler_h
#define LegacyProfiler_h

#include "Profile.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace JSC {

class DebuggerCallFrame;
class ExecState;
class VM;
class JSGlobalObject;
class JSObject;
class JSValue;
class ProfileGenerator;
struct CallIdentifier;

class LegacyProfiler {
    WTF_MAKE_FAST_ALLOCATED;
public:
    JS_EXPORT_PRIVATE static LegacyProfiler* profiler(); 
    static CallIdentifier createCallIdentifier(ExecState*, JSValue, const WTF::String& sourceURL, unsigned defaultLineNumber, unsigned defaultColumnNumber);

    JS_EXPORT_PRIVATE void startProfiling(ExecState*, const WTF::String& title);
    JS_EXPORT_PRIVATE PassRefPtr<Profile> stopProfiling(ExecState*, const WTF::String& title);
    void stopProfiling(JSGlobalObject*);

    void willExecute(ExecState* callerCallFrame, JSValue function);
    void willExecute(ExecState* callerCallFrame, const WTF::String& sourceURL, unsigned startingLineNumber, unsigned startingColumnNumber);
    void didExecute(ExecState* callerCallFrame, JSValue function);
    void didExecute(ExecState* callerCallFrame, const WTF::String& sourceURL, unsigned startingLineNumber, unsigned startingColumnNumber);

    void exceptionUnwind(ExecState* handlerCallFrame);

    void didPause(PassRefPtr<DebuggerCallFrame>);
    void didContinue(PassRefPtr<DebuggerCallFrame>);

    const Vector<RefPtr<ProfileGenerator>>& currentProfiles() { return m_currentProfiles; };

private:
    Vector<RefPtr<ProfileGenerator>> m_currentProfiles;
    static LegacyProfiler* s_sharedLegacyProfiler;
};

} // namespace JSC

#endif // LegacyProfiler_h

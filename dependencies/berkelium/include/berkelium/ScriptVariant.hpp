/*  Berkelium - Embedded Chromium
 *  ScriptVariant.hpp
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

#ifndef _BERKELIUM_SCRIPT_VARIANT_HPP_
#define _BERKELIUM_SCRIPT_VARIANT_HPP_
#include "berkelium/WeakString.hpp"

namespace Berkelium {
namespace Script {

class BERKELIUM_EXPORT Variant {
public:
	enum Type {
		JSSTRING,
		JSDOUBLE,
		JSBOOLEAN,
		JSNULL,
		JSEMPTYOBJECT,
		JSEMPTYARRAY,
		JSBINDFUNC,
		JSBINDSYNCFUNC
	};
private:
	union {
		WideString mStrPointer;
		double mDoubleValue;
	};

	Type mType;
	void initwc(const wchar_t* str, size_t length) ;
	void initmb(const char* str, size_t length);
	void initdbl(double dblval);
	void initbool(bool boolval);
	void initnull(Type typ);
	void initvariant(const Variant& other);
	void destroy();
	bool hasString() {
		return mType == JSSTRING || mType == JSBINDFUNC || mType == JSBINDSYNCFUNC;
	}
	Variant(Type type) {
		initnull(type);
	}
	Variant(WideString str, Type type);
public:
	Variant(const char* str);
	Variant(const wchar_t* str);
	Variant(WideString str);
	Variant(double dblval) {
		initdbl(dblval);
	}
	Variant(int intval) {
		initdbl(intval);
	}
	Variant(bool boolval) {
		initbool(boolval);
	}
	Variant() {
		initnull(JSNULL);
	}

	static Variant emptyArray();
	static Variant emptyObject();

	static Variant bindFunction(WideString name, bool synchronous) {
		return Variant(name, synchronous ? JSBINDSYNCFUNC: JSBINDFUNC);
	}

	Variant(const Variant& other);
	Variant& operator=(const Variant& other);

	bool toBoolean() const {
		if (mType == JSDOUBLE || mType == JSBOOLEAN) {
			return mDoubleValue != 0;
		} else if (mType == JSSTRING) {
			return mStrPointer.length() ? true : false;
		} else {
			return false;
		}
	}
	int toInteger() const {
		if (mType == JSDOUBLE || mType == JSBOOLEAN) {
			return (int)mDoubleValue;
		} else {
			return 0;
		}
	}
	double toDouble() const {
		if (mType == JSDOUBLE || mType == JSBOOLEAN) {
			return mDoubleValue;
		} else {
			return 0;
		}
	}

	WideString toString() const {
		if (mType == JSSTRING) {
			return mStrPointer;
		} else {
			return WideString::empty();
		}
	}

	WideString toFunctionName() const {
		if (mType == JSBINDFUNC || mType == JSBINDSYNCFUNC) {
			return mStrPointer;
		} else {
			return WideString::empty();
		}
	}

	Type type() const {
		return mType;
	}

	~Variant();
};

}
}

#endif

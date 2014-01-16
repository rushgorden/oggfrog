/*  @(#) $Id: ZTuple_Java.h,v 1.1 2003/10/15 23:40:13 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
http://www.zoolib.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------ */

#ifndef __ZTuple_Java__
#define __ZTuple_Java__ 1
#include "zconfig.h"

#include "ZTuple.h"
#include <jni.h>

namespace ZTuple_Java {

ZTupleValue sObjectToTupleValue(JNIEnv* iEnv, jobject iValue);

ZTuple sMapToTuple(JNIEnv* iEnv, jobject iMap);
ZTuple sMapToTuple(JNIEnv* iEnv, jclass iClass_Map, jobject iMap);

jobject sTupleValueToObject(JNIEnv* iEnv, const ZTupleValue& iTV);

jobject sTupleToMap(JNIEnv* iEnv, const ZTuple& iTuple);

} // namespace ZTuple_Java

#endif // __ZTuple_Java__

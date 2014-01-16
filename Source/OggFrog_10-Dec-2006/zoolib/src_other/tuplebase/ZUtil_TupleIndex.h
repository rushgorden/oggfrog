/*  @(#) $Id: ZUtil_TupleIndex.h,v 1.1 2006/10/13 20:23:54 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZUtil_TupleIndex__
#define __ZUtil_TupleIndex__
#include "zconfig.h"

#include "ZTupleIndex.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_TupleIndex

namespace ZUtil_TupleIndex {

ZRef<ZTupleIndexFactory> sCreate_General(const string& iPropName0);

ZRef<ZTupleIndexFactory> sCreate_General(
	const string& iPropName0,
	const string& iPropName1);

ZRef<ZTupleIndexFactory> sCreate_General(
	const string& iPropName0,
	const string& iPropName1,
	const string& iPropName2);

ZRef<ZTupleIndexFactory> sCreate_String(const string& iPropName);

} // namespace ZUtil_TupleIndex

#endif // __ZUtil_TupleIndex__

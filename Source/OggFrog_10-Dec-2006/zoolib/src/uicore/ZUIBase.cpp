static const char rcsid[] = "@(#) $Id: ZUIBase.cpp,v 1.3 2006/07/12 19:41:08 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2000 Andrew Green and Learning in Motion, Inc.
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

#include "ZUIBase.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIBase

ZThreadSafe_t ZUIBase::sChangeCount;

ZUIBase::ZUIBase()
	{
	ZThreadSafe_Set(fChangeCount, ZThreadSafe_IncReturnOld(sChangeCount));
	}

void ZUIBase::Changed()
	{
	ZThreadSafe_Set(fChangeCount, ZThreadSafe_IncReturnOld(sChangeCount));
	}

int ZUIBase::GetChangeCount() const
	{ return ZThreadSafe_Get(fChangeCount); }

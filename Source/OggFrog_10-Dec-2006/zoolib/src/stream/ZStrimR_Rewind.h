/*  @(#) $Id: ZStrimR_Rewind.h,v 1.5 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZStrimR_Rewind__
#define __ZStrimR_Rewind__ 1
#include "zconfig.h"

#include "ZStrim.h"

#include <string>

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimR_Rewind

class ZStreamRWPos;

class ZStrimR_Rewind : public ZStrimR
	{
public:
	ZStrimR_Rewind(const ZStrimR& iStrimSource, const ZStreamRWPos& iBuffer);
	~ZStrimR_Rewind();

// From ZStrimR
	virtual void Imp_ReadUTF32(UTF32* iDest, size_t iCount, size_t* oCount);

// Our protocol
	void Rewind();

	uint64 GetMark();
	void SetMark(uint64 iMark);

protected:
	const ZStrimR& fStrimSource;
	const ZStreamRWPos& fBuffer;
	};

// =================================================================================================

#endif // __ZStrimR_Boundary__

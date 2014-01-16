/*  @(#) $Id: ZStreamR_SkipAllOnDestroy.h,v 1.4 2006/04/27 03:19:32 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2004 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZStreamR_SkipAllOnDestroy__
#define __ZStreamR_SkipAllOnDestroy__ 1

#include "zconfig.h"

#include "ZStream.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_SkipAllOnDestroy

/// A read filter stream that when destroyed invokes SkipAll on its real stream.

class ZStreamR_SkipAllOnDestroy : public ZStreamR
	{
public:
	ZStreamR_SkipAllOnDestroy(const ZStreamR& iStreamR);
	~ZStreamR_SkipAllOnDestroy();

	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();

	virtual void Imp_CopyToDispatch(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_CopyTo(const ZStreamW& iStreamW, uint64 iCount,
		uint64* oCountRead, uint64* oCountWritten);

	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

protected:
	ZStreamR& fStreamR;
	};

// Example streamer template:
// typedef ZStreamerR_FT<ZStreamR_SkipAllOnDestroy> ZStreamerR_SkipAllOnDestroy;

#endif // __ZStreamR_SkipAllOnDestroy__

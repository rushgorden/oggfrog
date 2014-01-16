/*  @(#) $Id: ZAsset_POSIX.h,v 1.3 2006/04/10 20:44:21 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZAsset_POSIX__
#define __ZAsset_POSIX__ 1

#include "zconfig.h"

#if ZCONFIG(OS, POSIX)

#include "ZAsset_Std.h" // For ZAssetTree_Std_Memory

// =================================================================================================
#pragma mark -
#pragma mark * ZAssetTree_POSIX_MemoryMapped

class ZAssetTree_POSIX_MemoryMapped : public ZAssetTree_Std_Memory
	{
public:
	ZAssetTree_POSIX_MemoryMapped(int inFileDescriptor, bool inAdopt, size_t inStart, size_t inLength);
	virtual ~ZAssetTree_POSIX_MemoryMapped();

protected:
	void LoadUp(int inFileDescriptor, size_t inStart, size_t inLength);

	int fFileDescriptor;
	bool fAdopted;

	void* fMappedAddress;
	size_t fMappedLength;
	};

#endif // ZCONFIG(OS, POSIX)

#endif // __ZAsset_POSIX__

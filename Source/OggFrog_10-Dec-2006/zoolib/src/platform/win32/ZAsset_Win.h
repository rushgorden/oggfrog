/*  @(#) $Id: ZAsset_Win.h,v 1.6 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZAsset_Win__
#define __ZAsset_Win__ 1

#include "zconfig.h"

#if ZCONFIG(OS, Win32)

#include "ZAsset_Std.h"
#include "ZWinHeader.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZAssetTree_Win_MemoryMapped

class ZAssetTree_Win_MemoryMapped : public ZAssetTree_Std_Memory
	{
public:
	ZAssetTree_Win_MemoryMapped(HANDLE inFileHandle, bool inAdopt, size_t inStart, size_t inLength);
	virtual ~ZAssetTree_Win_MemoryMapped();

protected:
	void LoadUp(HANDLE inFileHandle, size_t inStart, size_t inLength);

	HANDLE fHANDLE_File;
	bool fAdopted;

	HANDLE fHANDLE_FileMapping;
	LPVOID fMappedAddress;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZAssetTree_Win_MultiResource

class ZStreamRPos_Win_MultiResource;

class ZAssetTree_Win_MultiResource : public ZAssetTree_Std_Stream
	{
public:
	ZAssetTree_Win_MultiResource(HMODULE inHMODULE, const std::string& inType, const std::string& inName);
	virtual ~ZAssetTree_Win_MultiResource();

protected:
	ZStreamRPos_Win_MultiResource* fStream_Resource;
	};

#endif // ZCONFIG(OS, Win32)

#endif // __ZAsset_Win__

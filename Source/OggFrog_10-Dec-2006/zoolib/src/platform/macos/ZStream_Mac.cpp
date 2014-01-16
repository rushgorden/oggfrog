static const char rcsid[] = "@(#) $Id: ZStream_Mac.cpp,v 1.9 2006/07/15 20:54:22 goingware Exp $";

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

#include "ZStream_Mac.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

#include "ZCompat_algorithm.h" // For min
#include "ZDebug.h"

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/Resources.h>
#else
#	include <Resources.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_Mac_PartialResource

ZStreamRPos_Mac_PartialResource::ZStreamRPos_Mac_PartialResource(Handle inResourceHandle, bool inAdopt)
	{
	fResourceHandle = inResourceHandle;

	// We must have a non-nil resource.
	ZAssertStop(2, inResourceHandle != nil);

	// And it must be unloaded.
	ZAssertStop(2, *inResourceHandle == nil);

	fAdopted = inAdopt;
	fPosition = 0;
	fSize = ::GetResourceSizeOnDisk(fResourceHandle);
	}

ZStreamRPos_Mac_PartialResource::~ZStreamRPos_Mac_PartialResource()
	{
	if (fAdopted)
		::ReleaseResource(fResourceHandle);
	}

void ZStreamRPos_Mac_PartialResource::Imp_Read(void* inDest, size_t inCount, size_t* outCountRead)
	{
	size_t countToRead = min(uint64(inCount), sDiffPosR(fSize, fPosition));
	::ReadPartialResource(fResourceHandle, fPosition, inDest, countToRead);
	fPosition += countToRead;
	if (outCountRead)
		*outCountRead = countToRead;
	}

void ZStreamRPos_Mac_PartialResource::Imp_Skip(uint64 inCount, uint64* outCountSkipped)
	{
	uint64 realSkip = min(inCount, sDiffPosR(fSize, fPosition));
	fPosition += realSkip;
	if (outCountSkipped)
		*outCountSkipped = realSkip;
	}

uint64 ZStreamRPos_Mac_PartialResource::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRPos_Mac_PartialResource::Imp_SetPosition(uint64 inPosition)
	{ fPosition = inPosition; }

uint64 ZStreamRPos_Mac_PartialResource::Imp_GetSize()
	{ return fSize; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_Mac_PartialResource

ZStreamerRPos_Mac_PartialResource::ZStreamerRPos_Mac_PartialResource(Handle inResourceHandle, bool inAdopt)
:	fStream(inResourceHandle, inAdopt)
	{}

ZStreamerRPos_Mac_PartialResource::~ZStreamerRPos_Mac_PartialResource()
	{}

// From ZStreamerRPos
const ZStreamRPos& ZStreamerRPos_Mac_PartialResource::GetStreamRPos()
	{ return fStream; }

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

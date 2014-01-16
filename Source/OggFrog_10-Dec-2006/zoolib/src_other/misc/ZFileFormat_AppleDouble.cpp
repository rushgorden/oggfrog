static const char rcsid[] = "@(#) $Id: ZFileFormat_AppleDouble.cpp,v 1.3 2006/07/23 21:57:38 agreen Exp $";

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

#include "ZFileFormat_AppleDouble.h"
#include "ZMemory.h"
#include "ZStreamR_Source.h"
#include "ZStreamRWPos_RAM.h"

using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * ZFileFormat_AppleDouble

ZFileFormat_AppleDouble::Writer::Writer()
	{
	fIsAppleSingle = false;
	}

void ZFileFormat_AppleDouble::Writer::SetIsAppleSingle(bool iIsAppleSingle)
	{
	fIsAppleSingle = iIsAppleSingle;
	}

void ZFileFormat_AppleDouble::Writer::Add(uint32 iEntryID, ZRef<ZStreamerRPos> iStreamerRPos)
	{
	Entry theEntry;
	theEntry.fID = iEntryID;
	theEntry.fData = iStreamerRPos;
	theEntry.fPadding = 0;
	fEntries.push_back(theEntry);
	}

void ZFileFormat_AppleDouble::Writer::Add(uint32 iEntryID, const ZStreamRPos& iStreamRPos)
	{
	ZRef<ZStreamerRWPos> theStreamer = new ZStreamerRWPos_T<ZStreamRWPos_RAM>;
	theStreamer->GetStreamW().CopyAllFrom(iStreamRPos);
	this->Add(iEntryID, theStreamer);
	}

void ZFileFormat_AppleDouble::Writer::Add(uint32 iEntryID, const ZStreamR& iStreamR, size_t iSize)
	{
	ZRef<ZStreamerRWPos> theStreamer = new ZStreamerRWPos_T<ZStreamRWPos_RAM>;
	theStreamer->GetStreamW().CopyFrom(iStreamR, iSize);
	this->Add(iEntryID, theStreamer);
	}

void ZFileFormat_AppleDouble::Writer::Add(uint32 iEntryID, const void* iSource, size_t iSize)
	{
	ZRef<ZStreamerRWPos> theStreamer = new ZStreamerRWPos_T<ZStreamRWPos_RAM>;
	theStreamer->GetStreamWPos().Write(iSource, iSize);
	this->Add(iEntryID, theStreamer);
	}

ZRef<ZStreamerRWPos> ZFileFormat_AppleDouble::Writer::Create(uint32 iEntryID)
	{
	ZRef<ZStreamerRWPos> theStreamer = new ZStreamerRWPos_T<ZStreamRWPos_RAM>;
	this->Add(iEntryID, theStreamer);
	return theStreamer;
	}

void ZFileFormat_AppleDouble::Writer::ToStream(const ZStreamW& iStreamW) const
	{
	// Magic number
	if (fIsAppleSingle)
		iStreamW.WriteUInt32(0x00051600);
	else
		iStreamW.WriteUInt32(0x00051607);

	// Version number
	iStreamW.WriteUInt32(0x00020000);

	// Home file system (now just filler)
	iStreamW.CopyFrom(ZStreamR_Source(), 16);

	// Number of entries
	iStreamW.WriteUInt16(fEntries.size());

	const size_t sizeHeader = 26;
	const size_t sizeTOC = 12 * fEntries.size();
	size_t accumulatedOffset = sizeHeader + sizeTOC;

	for (vector<Entry>::const_iterator i = fEntries.begin(); i != fEntries.end(); ++i)
		{
		// Entry ID
		iStreamW.WriteUInt32(i->fID);

		// Offset to data
		iStreamW.WriteUInt32(accumulatedOffset);

		// Size
		const size_t theSize = i->fData->GetStreamRPos().GetSize();
		iStreamW.WriteUInt32(theSize);
		accumulatedOffset += theSize + i->fPadding;
		}

	for (vector<Entry>::const_iterator i = fEntries.begin(); i != fEntries.end(); ++i)
		{
		const ZStreamRPos& theStreamRPos = i->fData->GetStreamRPos();
		uint64 oldPosition = theStreamRPos.GetPosition();
		theStreamRPos.SetPosition(0);
		iStreamW.CopyAllFrom(theStreamRPos);
		iStreamW.CopyFrom(ZStreamR_Source(), i->fPadding);
		theStreamRPos.SetPosition(oldPosition);
		}
	}

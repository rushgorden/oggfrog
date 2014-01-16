static const char rcsid[] = "@(#) $Id: ZStream_Win.cpp,v 1.12 2006/08/20 20:03:30 agreen Exp $";

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

#include "ZStream_Win.h"

#if ZCONFIG(OS, Win32)

#include "ZCompat_algorithm.h" // For lower_bound, min
#include "ZDebug.h"
#include "ZMemory.h" // For ZBlockCopy
#include "ZStream_Memory.h" // For ZStreamRPos_Memory
#include "ZUnicode.h"
#include "ZUtil_Win.h"

using std::min;
using std::runtime_error;
using std::string;
using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_Win_MultiResource

static HRSRC sFindResource(HMODULE inHMODULE, const string& inName, const string& inType)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::FindResourceW(inHMODULE, ZUnicode::sAsUTF16(inName).c_str(), ZUnicode::sAsUTF16(inType).c_str());
	else
		return ::FindResourceA(inHMODULE, inName.c_str(), inType.c_str());
	}

static string sReadZeroTerminatedString(const ZStreamR& inStream)
	{
	string theString;
	while (true)
		{
		char theChar = inStream.ReadInt8();
		if (theChar == 0)
			break;
		theString += theChar;
		}
	return theString;
	}

ZStreamRPos_Win_MultiResource::ZStreamRPos_Win_MultiResource(HMODULE inHMODULE, const string& inType, const string& inName)
	{
	ZAssertStop(2, inHMODULE);
	fHMODULE = inHMODULE;
	fHGLOBAL_Current = nil;
	fLPVOID_Current = nil;
	fPosition = 0;

	// Load the descriptor resource
	if (HRSRC descHRSRC = ::sFindResource(fHMODULE, inName, inType))
		{
		if (HGLOBAL descHGLOBAL = ::LoadResource(fHMODULE, descHRSRC))
			{
			try
				{
				LPVOID descAddress = ::LockResource(descHGLOBAL);
				ZStreamRPos_Memory theSIM(descAddress, SizeofResource(fHMODULE, descHRSRC));

				size_t accumulatedSize = 0;
				size_t resourceCount = theSIM.ReadUInt16();
				string resourceType = sReadZeroTerminatedString(theSIM);
				for (size_t x = 0; x < resourceCount; ++x)
					{
					string resourceName = sReadZeroTerminatedString(theSIM);
					if (HRSRC currentHRSRC = ::sFindResource(fHMODULE, resourceName, resourceType))
						{
						fVector_HRSRC.push_back(currentHRSRC);
						accumulatedSize += ::SizeofResource(fHMODULE, currentHRSRC);
						fVector_Ends.push_back(accumulatedSize);
						}
					else
						{
						throw runtime_error("ZStreamRPos_Win_MultiResource, couldn't load resource");
						}
					}
				}
			catch (...)
				{
				::FreeResource(descHGLOBAL);
				throw;
				}
			::FreeResource(descHGLOBAL);
			}
		}
	fIndex = fVector_Ends.size();
	}

ZStreamRPos_Win_MultiResource::~ZStreamRPos_Win_MultiResource()
	{
	if (fHGLOBAL_Current)
		::FreeResource(fHGLOBAL_Current);
	}

void ZStreamRPos_Win_MultiResource::Imp_Read(void* inDest, size_t inCount, size_t* outCountRead)
	{
	char* localDest = reinterpret_cast<char*>(inDest);
	while (inCount)
		{
		vector<size_t>::iterator theIter = upper_bound(fVector_Ends.begin(), fVector_Ends.end(), fPosition);
		if (theIter == fVector_Ends.end())
			break;
		size_t theIndex = theIter - fVector_Ends.begin();
		if (fIndex != theIndex)
			{
			fIndex = theIndex;
			if (fHGLOBAL_Current)
				::FreeResource(fHGLOBAL_Current);
			fHGLOBAL_Current = ::LoadResource(fHMODULE, fVector_HRSRC[fIndex]);
			fLPVOID_Current = ::LockResource(fHGLOBAL_Current);
			if (fIndex == 0)
				fBegin = 0;
			else
				fBegin = fVector_Ends[fIndex - 1];
			fEnd = fVector_Ends[fIndex];
			}

		size_t countToMove = min(uint64(inCount), sDiffPosR(fEnd, fPosition));
		if (countToMove == 0)
			break;
		ZBlockCopy(reinterpret_cast<char*>(fLPVOID_Current) + fPosition - fBegin, localDest, countToMove);
		fPosition += countToMove;
		localDest += countToMove;
		inCount -= countToMove;
		}
	if (outCountRead)
		*outCountRead = localDest - reinterpret_cast<char*>(inDest);
	}

void ZStreamRPos_Win_MultiResource::Imp_Skip(uint64 inCount, uint64* outCountSkipped)
	{
	uint64 realSkip = min(inCount, sDiffPosR(fVector_Ends.back(), fPosition));
	fPosition += realSkip;
	if (outCountSkipped)
		*outCountSkipped = realSkip;
	}

uint64 ZStreamRPos_Win_MultiResource::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRPos_Win_MultiResource::Imp_SetPosition(uint64 inPosition)
	{ fPosition = inPosition; }

uint64 ZStreamRPos_Win_MultiResource::Imp_GetSize()
	{ return fVector_Ends.back(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerRPos_Win_MultiResource

ZStreamerRPos_Win_MultiResource::ZStreamerRPos_Win_MultiResource(HMODULE inHMODULE, const string& inType, const string& inName)
:	fStream(inHMODULE, inType, inName)
	{}

ZStreamerRPos_Win_MultiResource::~ZStreamerRPos_Win_MultiResource()
	{}

const ZStreamRPos& ZStreamerRPos_Win_MultiResource::GetStreamRPos()
	{ return fStream; }

#endif // ZCONFIG(OS, Win32)

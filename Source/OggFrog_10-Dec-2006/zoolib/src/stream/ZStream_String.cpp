static const char rcsid[] = "@(#) $Id: ZStream_String.cpp,v 1.4 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZStream_String.h"

#include "ZCompat_algorithm.h"
#include "ZDebug.h"
#include "ZMemory.h" // For ZBlockCopy

using std::min;
using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRPos_String

ZStreamRPos_String::ZStreamRPos_String(const string& inString)
:	fString(inString),
	fPosition(0)
	{}

ZStreamRPos_String::~ZStreamRPos_String()
	{}

void ZStreamRPos_String::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (size_t countToMove = min(uint64(iCount), sDiffPosR(fString.size(), fPosition)))
		{
		ZBlockCopy(&fString.at(fPosition), iDest, countToMove);
		fPosition += countToMove;
		if (oCountRead)
			*oCountRead = countToMove;
		}
	else if (oCountRead)
		{
		*oCountRead = 0;
		}
	}

uint64 ZStreamRPos_String::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRPos_String::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamRPos_String::Imp_GetSize()
	{ return fString.size(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamWPos_String

ZStreamWPos_String::ZStreamWPos_String(string& inString)
:	fString(inString),
	fPosition(0)
	{}

ZStreamWPos_String::~ZStreamWPos_String()
	{}

void ZStreamWPos_String::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	size_t neededSpace = sClampedW(fPosition + iCount);
	if (fString.size() < neededSpace)
		{
		try
			{
			fString.resize(neededSpace);
			}
		catch (...)
			{}
		}

	if (size_t countToMove = min(uint64(iCount), sDiffPosW(fString.size(), fPosition)))
		{
		ZBlockCopy(iSource, &fString.at(fPosition), countToMove);
		fPosition += countToMove;
		if (oCountWritten)
			*oCountWritten = countToMove;
		}
	else if (oCountWritten)
		{
		*oCountWritten = 0;
		}
	}

uint64 ZStreamWPos_String::Imp_GetPosition()
	{ return fPosition; }

void ZStreamWPos_String::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamWPos_String::Imp_GetSize()
	{ return fString.size(); }

void ZStreamWPos_String::Imp_SetSize(uint64 iSize)
	{
	size_t realSize = iSize;
	if (realSize != iSize)
		sThrowBadSize();

	fString.resize(realSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamRWPos_String

ZStreamRWPos_String::ZStreamRWPos_String(string& inString)
:	fString(inString),
	fPosition(0)
	{}

ZStreamRWPos_String::~ZStreamRWPos_String()
	{}

void ZStreamRWPos_String::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	if (size_t countToMove = min(uint64(iCount), sDiffPosR(fString.size(), fPosition)))
		{
		ZBlockCopy(&fString.at(fPosition), iDest, countToMove);
		fPosition += countToMove;
		if (oCountRead)
			*oCountRead = countToMove;
		}
	else if (oCountRead)
		{
		*oCountRead = 0;
		}
	}

void ZStreamRWPos_String::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	size_t neededSpace = sClampedW(fPosition + iCount);
	if (fString.size() < neededSpace)
		{
		try
			{
			fString.resize(neededSpace);
			}
		catch (...)
			{}
		}

	if (size_t countToMove = min(uint64(iCount), sDiffPosW(fString.size(), fPosition)))
		{
		ZBlockCopy(iSource, &fString.at(fPosition), countToMove);
		fPosition += countToMove;
		if (oCountWritten)
			*oCountWritten = countToMove;
		}
	else if (oCountWritten)
		{
		*oCountWritten = 0;
		}
	}

void ZStreamRWPos_String::Imp_Flush()
	{}

uint64 ZStreamRWPos_String::Imp_GetPosition()
	{ return fPosition; }

void ZStreamRWPos_String::Imp_SetPosition(uint64 iPosition)
	{ fPosition = iPosition; }

uint64 ZStreamRWPos_String::Imp_GetSize()
	{ return fString.size(); }

void ZStreamRWPos_String::Imp_SetSize(uint64 iSize)
	{
	size_t realSize = iSize;
	if (realSize != iSize)
		sThrowBadSize();

	fString.resize(realSize);
	}

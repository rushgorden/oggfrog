static const char rcsid[] = "@(#) $Id: ZStream_ASCIIStrim.cpp,v 1.1 2006/04/11 12:00:50 agreen Exp $";

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

#include "ZStream_ASCIIStrim.h"
#include "ZStrim.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_ASCIIStrim

ZStreamR_ASCIIStrim::ZStreamR_ASCIIStrim(const ZStrimR& iStrimR)
:	fStrimR(iStrimR)
	{}

void ZStreamR_ASCIIStrim::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	UTF8* localDest = reinterpret_cast<UTF8*>(iDest);

	while (iCount)
		{
		// Top up our buffer with UTF8 code points.
		size_t countRead;
		fStrimR.Read(localDest, iCount, &countRead);
		if (countRead == 0)
			break;

		// Scan till we find a non-ASCII code point (if any).
		for (UTF8* readFrom = localDest, *destEnd = localDest + countRead;
			readFrom < destEnd; ++readFrom)
			{
			if (*readFrom & 0x80)
				{
				// We found a problem, so start transcribing only those bytes that are okay.
				for (UTF8* writeTo = readFrom; readFrom < destEnd; ++readFrom)
					{
					if (!(*readFrom & 0x80))
						*writeTo++ = *readFrom;
					}
				break;
				}
			}
		localDest += countRead;
		iCount -= countRead;
		}

	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<UTF8*>(iDest);
	}

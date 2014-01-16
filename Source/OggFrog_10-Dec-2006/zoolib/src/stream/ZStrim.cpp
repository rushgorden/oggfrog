static const char rcsid[] = "@(#) $Id: ZStrim.cpp,v 1.26 2006/07/23 21:58:20 agreen Exp $";

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

#include "ZStrim.h"

#include "ZCompat_algorithm.h"
#include "ZDebug.h"
#include "ZMemory.h"

#include <cstdarg>
#include <cstdio>

using std::min;
using std::nothrow;
using std::range_error;
using std::string;

#if ZCONFIG(Compiler, MSVC)
#	define vsnprintf _vsnprintf
#elif !ZCONFIG(Compiler, GCC) || _GLIBCPP_USE_C99
//	using std::vsnprintf;
#endif

#define kDebug_Strim 2

/**
\defgroup group_Strim Strims
\sa ZUnicode
\sa group_Unicode

The word \e strim was coined by Eric Cooper. It's a nice punchy designator
for a <i>string stream</i>, an interface that is akin to a stream but geared
towards the reading and writing of unicode strings. In working with strims
you should understand the terms <i>code unit</i> and <i>code point</i>, which
are covered in the documentation sections \ref group_Unicode and \ref ZUnicode.

If you're working with ASCII or other single-byte encoding of text then the
ZStreamR and ZStreamW facilities are sufficient. If you'd like to be able
to handle arbitrary text encodings then you should consider using the ZStrimR
and ZStrimW interfaces. Strings in any of the three Unicode serialization forms
(UTF-8, UTF-16, UTF-32) can be passed to a ZStrimW, and ZStrimR returns
strings in any of the serialization forms. Both can limit their operation by
code unit and code point counts.

As ZStrimR and ZStrimW are abstract interfaces you'll never instantiate
them directly. Instead you'll use a concrete subclass that has overrides
of the pure virtual methods that are ultimately responsible for taking
a string and disposing of it somewhere, or sourcing text and returning it.

For example, if you have a ZStreamerRW that encapsulates a network connection,
and know that the other end is expecting little endian UTF-16 text, you can
do this:

\code
	ZStrimW_StreamUTF16LE theStrim(theStreamerRW->GetStreamW());
	theStrim.Write("This is some UTF-8 text");
	theStrim.Write("but will be written to theStreamerRW as UTF-16LE.");
\endcode

if it had been expecting UTF-8 text you could do this:

\code
	ZStrimW_StreamUTF8 theStrim(theStreamerRW->GetStreamW());
	theStrim.Write(L"This is some native endian UTF-16 text");
	theStrim.Write(L"but will be written to theStreamerRW as UTF-8.");
\endcode

In fact in the latter case the string is of type wchar_t, and thus might be a
sequence of 32 bit code points, so the UTF32* entry point would be invoked
rather than the UTF16* entry point. In either case the program at the other end
of the network connection would see text that was encoded appropriately.

\note A short read is not necessarily indicative of an end of stream condition.
If the buffer does not have enough space for enough code units to make up a
complete code point then the read will return without reading any of that final
code point's code units. For this reason, when reading UTF-8, you should always
pass a buffer that is at least six code units in length, and when reading UTF-16
one that is at least two code units in length.
*/

static const size_t kBufSize = ZooLib::sStackBufferSize;

// =================================================================================================
#pragma mark -
#pragma mark * Utility methods

/**
Copy data from \a iStrimR to \a iStrimW by reading it into a buffer and writing
from that buffer.
*/
static void sCopyReadToWrite(const ZStrimR& iStrimR, const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	if (oCountCPRead)
		*oCountCPRead = 0;
	if (oCountCPWritten)
		*oCountCPWritten = 0;

	if (iCountCP == 0)
		return;

	if (iCountCP > ZooLib::sStackBufferSize)
		{
		// Try to allocate and use an 8K heap-based buffer.
		if (UTF32* heapBuffer = new(nothrow) UTF32[2048])
			{
			try
				{
				size_t cpRemaining = iCountCP;
				while (cpRemaining > 0)
					{
					size_t cpRead;
					iStrimR.Read(heapBuffer, min(cpRemaining, size_t(2048)), &cpRead);
					if (cpRead == 0)
						break;
					if (oCountCPRead)
						*oCountCPRead += cpRead;
					cpRemaining -= cpRead;
					UTF32* tempSource = heapBuffer;
					while (cpRead > 0)
						{
						size_t cpWritten;
						iStrimW.Write(tempSource, cpRead, &cpWritten);
						if (cpWritten == 0)
							{
							cpRemaining = 0;
							break;
							}
						tempSource += cpWritten;
						cpRead -= cpWritten;
						if (oCountCPWritten)
							*oCountCPWritten += cpWritten;
						}
					}
				}
			catch (...)
				{
				delete[] heapBuffer;
				throw;
				}
			delete[] heapBuffer;
			return;
			}
		}

	UTF32 localBuffer[ZooLib::sStackBufferSize];
	size_t cpRemaining = iCountCP;
	while (cpRemaining > 0)
		{
		size_t cpRead;
		iStrimR.Read(localBuffer, min(cpRemaining, ZooLib::sStackBufferSize), &cpRead);
		if (cpRead == 0)
			break;
		if (oCountCPRead)
			*oCountCPRead += cpRead;
		cpRemaining -= cpRead;
		UTF32* tempSource = localBuffer;
		while (cpRead > 0)
			{
			size_t cpWritten;
			iStrimW.Write(tempSource, cpRead, &cpWritten);
			if (cpWritten == 0)
				{
				cpRemaining = 0;
				break;
				}
			tempSource += cpWritten;
			cpRead -= cpWritten;
			if (oCountCPWritten)
				*oCountCPWritten += cpWritten;
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimR
/**
\class ZStrimR
\nosubgrouping
\ingroup group_Strim
\ingroup group_Unicode
\sa group_Strim
\sa group_Unicode
*/

/// Read a single CP, returning false if the end of the strim has been reached.
/** Read a single CP from the strim and place it in \a oCP. If the CP was
successfully read then true is returned, otherwise false is returned
and the value of \a oCP is undefined.
*/
bool ZStrimR::ReadCP(UTF32& oCP) const
	{
	for (;;)
		{
		size_t countRead;
		this->Read(&oCP, 1, &countRead);
		if (countRead == 0)
			return false;
		if (ZUnicode::sIsValid(oCP))
			return true;
		}
	}

/// Read a single CP and return it, throwing end of strim if necessary.
UTF32 ZStrimR::ReadCP() const
	{
	UTF32 theCP;
	size_t countRead;
	this->Read(&theCP, 1, &countRead);
	if (countRead == 0)
		sThrowEndOfStrim();		
	return theCP;
	}

/** Read \a iCountCP code points and return them in a string32.
May throw an end of read strim exception.
*/
string32 ZStrimR::ReadUTF32(size_t iCountCP) const
	{
	string32 result;
	size_t destGenerated = 0;
	while (iCountCP)
		{
		result.resize(destGenerated + iCountCP);
		size_t cuRead;
		UTF32* dest = const_cast<UTF32*>(result.data()) + destGenerated;
		this->Read(dest, iCountCP, &cuRead);
		size_t cpRead = ZUnicode::sCUToCP(dest, cuRead);
		if (cpRead == 0)
			sThrowEndOfStrim();
		iCountCP -= cpRead;
		destGenerated += cuRead;
		}
	result.resize(destGenerated);
	return result;
	}


/** Read \a iCountCP code points and return them in a string16.
May throw an end of read strim exception.
*/
string16 ZStrimR::ReadUTF16(size_t iCountCP) const
	{
	string16 result;
	size_t destGenerated = 0;
	while (iCountCP)
		{
		result.resize(destGenerated + iCountCP + 1);
		size_t cuRead;
		size_t cpRead;
		this->Read(const_cast<UTF16*>(result.data()) + destGenerated,
			iCountCP + 1, &cuRead, iCountCP, &cpRead);

		if (cpRead == 0)
			sThrowEndOfStrim();
		iCountCP -= cpRead;
		destGenerated += cuRead;
		}
	result.resize(destGenerated);
	return result;
	}


/** Read \a iCountCP code points and return them in a string.
May throw an end of read strim exception.
*/
string8 ZStrimR::ReadUTF8(size_t iCountCP) const
	{
	string8 result;
	size_t destGenerated = 0;
	while (iCountCP)
		{
		result.resize(destGenerated + iCountCP + 5);
		size_t cuRead;
		size_t cpRead;
		this->Read(const_cast<UTF8*>(result.data()) + destGenerated,
			iCountCP + 5, &cuRead, iCountCP, &cpRead);

		if (cpRead == 0)
			sThrowEndOfStrim();
		iCountCP -= cpRead;
		destGenerated += cuRead;
		}
	result.resize(destGenerated);
	return result;
	}


/** Read \a iCount code units into the memory starting at \a iDest and
extending to \a iDest + \a iCount. The number of code units actually
read is placed in oCount.*/
void ZStrimR::Read(UTF32* iDest, size_t iCount, size_t* oCount) const
	{ const_cast<ZStrimR*>(this)->Imp_ReadUTF32(iDest, iCount, oCount); }

/// \overload
void ZStrimR::Read(UTF16* iDest, size_t iCountCU, size_t* oCountCU) const
	{ const_cast<ZStrimR*>(this)->Imp_ReadUTF16(iDest, iCountCU, oCountCU, iCountCU, nil); }

/// \overload
void ZStrimR::Read(UTF8* iDest, size_t iCountCU, size_t* oCountCU) const
	{ const_cast<ZStrimR*>(this)->Imp_ReadUTF8(iDest, iCountCU, oCountCU, iCountCU, nil); }


/** Read code points into the buffer at \a iDest. No more than \a iCountCU code units will
be read, and no more code units than make up \a iCountCP code points will be read. If
\a oCountCU is non-nil then the actual number of code units will be placed in \a *oCountCU. If
\a oCountCP is non-nil then the actual number of code points will be placed in \a *oCountCP. Note
that some strim implementations generate the number of code points as a natural side effect
of their operation, whereas for others its necessary that they examine the read data, adding
some overhead. If you don't \em need to know the number of code points then it is best to
pass nil for \a oCountCP.
*/
void ZStrimR::Read(UTF16* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP) const
	{ const_cast<ZStrimR*>(this)->Imp_ReadUTF16(iDest, iCountCU, oCountCU, iCountCP, oCountCP); }

/// \overload
void ZStrimR::Read(UTF8* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP) const
	{ const_cast<ZStrimR*>(this)->Imp_ReadUTF8(iDest, iCountCU, oCountCU, iCountCP, oCountCP); }


/// Read data from this strim and write it to \a iStrimW until this strim reaches its end.
void ZStrimR::CopyAllTo(const ZStrimW& iStrimW) const
	{
	size_t countRead, countWritten;
	this->CopyAllTo(iStrimW, &countRead, &countWritten);
	if (countRead > countWritten)
		ZStrimW::sThrowEndOfStrim();
	}


/// Read data from this strim and write it to \a iStrimW until one or other strim reaches its end.
/**
\param iStrimW The strim to which the code points should be written.
\param oCountCPRead (optional output) The number of code points that were actually read.
\param oCountCPWritten (optional output) The number of code points that were actually written.

\a oCountCPWritten will be less than or equal to \a oCountCPRead. If \a oCountCPWritten is less than
\a oCountCPRead it's a strong indication that \a iStrimW has reached its end, and a subsequent call
to \c CopyTo or the \a iStrimW's \c Write will likely return zero in \a oCountCPWritten.
*/
void ZStrimR::CopyAllTo(const ZStrimW& iStrimW, size_t* oCountCPRead, size_t* oCountCPWritten) const
	{
	if (oCountCPRead)
		*oCountCPRead = 0;

	if (oCountCPWritten)
		*oCountCPWritten = 0;

	for (;;)
		{
		size_t cpRead, cpWritten;
		this->CopyTo(iStrimW, 128*1024*1024, &cpRead, &cpWritten);

		if (oCountCPRead)
			*oCountCPRead += cpRead;

		if (oCountCPWritten)
			*oCountCPWritten += cpWritten;

		if (cpRead != cpWritten || cpRead == 0 || cpWritten == 0)
			break;
		}
	}


/// Read data from this strim and write it to \a iStrimW.
/**
\param iStrimW The strim to which the data should be written.
\param iCountCP The number of code points to be read from this strimg and written to \a iStrimgW.

If \a iCount bytes could not be read then an end of read strim exception is thrown. If
\a iCount bytes could not be written then an end of write strim exception is thrown.
*/
void ZStrimR::CopyTo(const ZStrimW& iStrimW, size_t iCountCP) const
	{
	size_t cpRead, cpWritten;
	const_cast<ZStrimR*>(this)->Imp_CopyToDispatch(iStrimW, iCountCP, &cpRead, &cpWritten);
	if (cpRead != iCountCP)
		sThrowEndOfStrim();
	if (cpWritten != iCountCP)
		ZStrimW::sThrowEndOfStrim();
	}


/// Read data from this strimg and write it to \a iStrimW.
/**
\param iStrimW The strim to which the data should be written.
\param iCountCP The number of code points to be read from this strim and written to \a iStrimW.
\param oCountCPRead (optional output) The number of code points that were actually read.
\param oCountCPWritten (optional output) The number of code points that were actually written.
*/
void ZStrimR::CopyTo(const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten) const
	{
	const_cast<ZStrimR*>(this)->Imp_CopyToDispatch(iStrimW,
		iCountCP, oCountCPRead, oCountCPWritten);
	}


/// Skip over \a iCount code points.
/**
\param iCountCP The number of bytes to be skipped over. If the strim reaches its
end prematurely then an end of strim exception is thrown.
*/
void ZStrimR::Skip(size_t iCountCP) const
	{
	size_t actualCount;
	this->Skip(iCountCP, &actualCount);
	if (actualCount != iCountCP)
		sThrowEndOfStrim();
	}


/// Attempt to skip bytes, return the count actually skipped.
void ZStrimR::Skip(size_t iCountCP, size_t* oCountCPSkipped) const
	{ const_cast<ZStrimR*>(this)->Imp_Skip(iCountCP, oCountCPSkipped); }

/// Skip code points until the end of the strim is reached.
void ZStrimR::SkipAll() const
	{ this->SkipAll(nil); }


/// Skip code points until the end of the strim is reached, return the count actually skipped.
/**
\param oCountCPSkipped (optional output) The number of code points actually skipped. If zero
is returned then the end of the strim had already been reached.
*/
void ZStrimR::SkipAll(size_t* oCountCPSkipped) const
	{
	if (oCountCPSkipped)
		*oCountCPSkipped = 0;

	for (;;)
		{
		size_t countSkipped;
		this->Skip(128*1024*1024, &countSkipped);

		if (countSkipped == 0)
			break;

		if (oCountCPSkipped)
			*oCountCPSkipped += countSkipped;
		}
	}

void ZStrimR::Imp_ReadUTF16(UTF16* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	UTF32 utf32Buffer[kBufSize];
	UTF16* localDest = iDest;
	size_t localCountCP = iCountCP;
	// If we've got space for two code units we are assured of
	// being able to read at least one code point.
	while (iCountCU >= 2 && localCountCP)
		{
		size_t utf32Read;
		this->Read(utf32Buffer, min(kBufSize, min(localCountCP, iCountCU)), &utf32Read);
		if (utf32Read == 0)
			break;

		size_t utf32Consumed;
		size_t utf16Generated;
		size_t cpGenerated;
		ZUnicode::sUTF32ToUTF16(utf32Buffer, utf32Read, &utf32Consumed,
			localDest, iCountCU, &utf16Generated, &cpGenerated);

		ZAssertStop(kDebug_Strim, utf32Consumed == utf32Read);

		localCountCP -= cpGenerated;
		iCountCU -= utf16Generated;
		localDest += utf16Generated;
		}
	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;
	}

void ZStrimR::Imp_ReadUTF8(UTF8* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	UTF32 utf32Buffer[kBufSize];
	UTF8* localDest = iDest;
	size_t localCountCP = iCountCP;
	// If we've got space for six code units we are assured of
	// being able to read at least one code point.
	while (iCountCU >= 6 && localCountCP)
		{
		size_t utf32Read;
		this->Read(utf32Buffer, min(kBufSize, min(localCountCP, iCountCU)), &utf32Read);
		if (utf32Read == 0)
			break;

		size_t utf32Consumed;
		size_t utf8Generated;
		size_t cpGenerated;
		ZUnicode::sUTF32ToUTF8(utf32Buffer, utf32Read, &utf32Consumed,
			localDest, iCountCU, &utf8Generated, &cpGenerated);

		ZAssertStop(kDebug_Strim, utf32Consumed == utf32Read);

		localCountCP -= cpGenerated;
		iCountCU -= utf8Generated;
		localDest += utf8Generated;
		}
	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;
	}


/// Invoke \a iStrimW's \c Imp_CopyFrom method.
/**
The first part of the two-stage dispatch initiated by \c CopyTo and \c CopyAllTo. Override this
method if your strim's data is in memory and thus can be written to \a iStrimW by a single
call to its \c Write method.
*/
void ZStrimR::Imp_CopyToDispatch(const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{ const_cast<ZStrimW&>(iStrimW).Imp_CopyFrom(*this, iCountCP, oCountCPRead, oCountCPWritten); }


/// Actually copy data from this strim to \a iStrimW.
/**
The second part of the two-stage dispatch initiated by \c CopyTo and \c CopyAllTo. The default
implementation calls \c sCopyReadToWrite. Override this method if your strim's data is in memory
and thus can be written to \a iStrimW by a single call to its \c Write method.
\sa sCopyReadToWrite
*/
void ZStrimR::Imp_CopyTo(const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{ sCopyReadToWrite(*this, iStrimW, iCountCP, oCountCPRead, oCountCPWritten); }


/// Read and discard iCountCP code points.
/**
Override this method if your strim implementation can skip past code points without actually
reading them.
\param iCountCP Count of code points to be skipped.
\param oCountCPSkipped Number of code points that were actually skipped. If iCountCP is non zero and
oCountCPSkipped is set to zero then this indicates that the strim has reached its end.
*/
void ZStrimR::Imp_Skip(size_t iCountCP, size_t* oCountCPSkipped)
	{
	size_t cpRemaining = iCountCP;
	while (cpRemaining)
		{
		UTF32 buffer[ZooLib::sStackBufferSize];
		size_t cpRead;
		this->Read(buffer, min(cpRemaining, ZooLib::sStackBufferSize), &cpRead);
		if (cpRead == 0)
			break;
		cpRemaining -= cpRead;
		}
	if (oCountCPSkipped)
		*oCountCPSkipped = iCountCP - cpRemaining;
	}

void ZStrimR::sThrowEndOfStrim()
	{ throw ExEndOfStrim(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimR::ExEndOfStrim

ZStrimR::ExEndOfStrim::ExEndOfStrim()
:	range_error("ZStrimR::ExEndOfStrim")
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimU
/**
\class ZStrimU
\nosubgrouping
\ingroup group_Strim
\ingroup group_Unicode
\sa group_Strim
\sa group_Unicode

A ZStrimU is a read strim with the addition of an Unread method. Unread causes the last code point
read to be returned as the first code point in any subsequent read request. Think \c ungetc.
*/

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW

/**
\class ZStrimW
\nosubgrouping
\ingroup group_Strim
\ingroup group_Unicode
\sa group_Strim
\sa group_Unicode
*/

/** Write the single code point \a iCP.
May throw an end of write strim exception.
*/
void ZStrimW::WriteCP(UTF32 iCP) const
	{
	this->Write(&iCP, 1);
	}

/** Write the zero terminated UTF-32 string starting at \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF32* iString) const
	{
	if (size_t length = ZUnicode::sCountCU(iString))
		this->Write(iString, length);
	}

/** Write the zero terminated UTF-16 string starting at \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF16* iString) const
	{
	if (size_t length = ZUnicode::sCountCU(iString))
		this->Write(iString, length);
	}

/** Write the zero terminated UTF-8 string starting at \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF8* iString) const
	{
	if (size_t length = ZUnicode::sCountCU(iString))
		this->Write(iString, length);
	}

/** Write the UTF-32 string in \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const string32& iString) const
	{
	if (size_t length = iString.size())
		this->Write(iString.data(), length);
	}

/** Write the UTF-16 string in \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const string16& iString) const
	{
	if (size_t length = iString.size())
		{
		const UTF16* source = iString.data();
		size_t cuConsumed;
		this->Write(source, length, &cuConsumed);
		if (length != cuConsumed)
			{
			// See comments in the UTF-8 variant.
			if (0 != ZUnicode::sCUToCP(source + cuConsumed, source + length))
				sThrowEndOfStrim();
			}
		}
	}

/** Write the UTF-8 string in \a iString.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const string8& iString) const
	{
	if (size_t length = iString.size())
		{
		const UTF8* source = iString.data();
		size_t cuConsumed;
		this->Write(source, length, &cuConsumed);
		if (length != cuConsumed)
			{
			// We weren't able to write the whole string.
			// It may be that the final code units are a valid prefix and thus
			// were not consumed by Imp_WriteUTF8 because it's expecting
			// that the caller will pass the prefix again, this time with enough
			// following code units to make up a complete code point,
			// or with invalid code units which will be consumed and ignored.
			// For string8 (and string16 and string32) we know there's
			// nothing more to come. So if the string contains no more
			// complete valid code points between source + cuConsumed and the
			// end then we can just return. If there are one or more valid
			// code points then the short write must be because we've reached
			// our end, and we throw the end of strim exception.
			if (0 != ZUnicode::sCUToCP(source + cuConsumed, source + length))
				sThrowEndOfStrim();
			}
		}
	}

/** Write the UTF-32 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF32* iSource, size_t iCountCU) const
	{
	size_t cuWritten;
	this->Write(iSource, iCountCU, &cuWritten);
	if (cuWritten != iCountCU)
		sThrowEndOfStrim();
	}

/** Write the UTF-16 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF16* iSource, size_t iCountCU) const
	{
	size_t cuWritten;
	this->Write(iSource, iCountCU, &cuWritten);
	if (cuWritten != iCountCU)
		sThrowEndOfStrim();
	}

/** Write the UTF-8 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
May throw an end of write strim exception.
*/
void ZStrimW::Write(const UTF8* iSource, size_t iCountCU) const
	{
	size_t cuWritten;
	this->Write(iSource, iCountCU, &cuWritten);
	if (cuWritten != iCountCU)
		sThrowEndOfStrim();
	}

/** Write the UTF-32 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
Report the number of code units succesfully written in the optional output parameter \a oCountCU.
*/
void ZStrimW::Write(const UTF32* iSource, size_t iCountCU, size_t* oCountCU) const
	{ const_cast<ZStrimW*>(this)->Imp_WriteUTF32(iSource, iCountCU, oCountCU); }

/** Write the UTF-16 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
Report the number of code units succesfully written in the optional output parameter \a oCountCU.
*/
void ZStrimW::Write(const UTF16* iSource, size_t iCountCU, size_t* oCountCU) const
	{ const_cast<ZStrimW*>(this)->Imp_WriteUTF16(iSource, iCountCU, oCountCU); }

/** Write the UTF-8 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
 Report the number of code units succesfully written in the optional output parameter \a oCountCU.
*/
void ZStrimW::Write(const UTF8* iSource, size_t iCountCU, size_t* oCountCU) const
	{ const_cast<ZStrimW*>(this)->Imp_WriteUTF8(iSource, iCountCU, oCountCU); }

/** Write the UTF-32 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
Do not write more code units than those required to represent \a iCountCP code points.
Report the number of code units and code points succesfully written in the optional output
parameters \a oCountCU and \a oCountCP.
*/
void ZStrimW::Write(const UTF32* iSource,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP) const
	{
	size_t actualCU;
	if (iCountCP >= iCountCU)
		{
		// iCountCP is the same or larger than iCountCU, and thus the limit
		// on what can be written is simply iCountCU because a single code unit
		// maps to at most one code point.
		actualCU = iCountCU;
		}
	else
		{
		// We're being asked to write fewer CPs than we have CUs. So we have to
		// map from CPs to CUs.
		actualCU = min(iCountCU, ZUnicode::sCPToCU(iSource, iCountCU, iCountCP, nil));
		}

	if (oCountCU || oCountCP)
		{
		size_t cuWritten;
		this->Write(iSource, actualCU, &cuWritten);
		if (oCountCU)
			*oCountCU = cuWritten;
		if (oCountCP)
			*oCountCP = ZUnicode::sCUToCP(iSource, cuWritten);
		}
	else
		{
		this->Write(iSource, actualCU, nil);
		}
	}

/** Write the UTF-16 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
Do not write more code units than those required to represent \a iCountCP code points.
Report the number of code units and code points succesfully written in the optional output
parameters \a oCountCU and \a oCountCP.
*/
void ZStrimW::Write(const UTF16* iSource,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP) const
	{
	size_t actualCU;
	if (iCountCP >= iCountCU)
		{
		// iCountCP is the same or larger than iCountCU, and thus the limit
		// on what can be written is simply iCountCU because a single code unit
		// maps to at most one code point.
		actualCU = iCountCU;
		}
	else
		{
		// We're being asked to write fewer CPs than we have CUs. So we have to
		// map from CPs to CUs.
		actualCU = min(iCountCU, ZUnicode::sCPToCU(iSource, iCountCU, iCountCP, nil));
		}

	if (oCountCU || oCountCP)
		{
		size_t cuWritten;
		this->Write(iSource, actualCU, &cuWritten);
		if (oCountCU)
			*oCountCU = cuWritten;
		if (oCountCP)
			*oCountCP = ZUnicode::sCUToCP(iSource, cuWritten);
		}
	else
		{
		this->Write(iSource, actualCU, nil);
		}
	}

/** Write the UTF-8 string starting at \a iSource and continuing to \a iSource + \a iCountCU.
Do not write more code units than those required to represent \a iCountCP code points.
Report the number of code units and code points succesfully written in the optional output
parameters \a oCountCU and \a oCountCP.
*/
void ZStrimW::Write(const UTF8* iSource,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP) const
	{
	size_t actualCU;
	if (iCountCP >= iCountCU)
		{
		// iCountCP is the same or larger than iCountCU, and thus the limit
		// on what can be written is simply iCountCU because a single code unit
		// maps to at most one code point.
		actualCU = iCountCU;
		}
	else
		{
		// We're being asked to write fewer CPs than we have CUs. So we have to
		// map from CPs to CUs.
		actualCU = min(iCountCU, ZUnicode::sCPToCU(iSource, iCountCU, iCountCP, nil));
		}

	if (oCountCU || oCountCP)
		{
		size_t cuWritten;
		this->Write(iSource, actualCU, &cuWritten);
		if (oCountCU)
			*oCountCU = cuWritten;
		if (oCountCP)
			*oCountCP = ZUnicode::sCUToCP(iSource, cuWritten);
		}
	else
		{
		this->Write(iSource, actualCU, nil);
		}
	}

/** Write the zero-terminated UTF-8 string starting at \a iString. Standard printf-style parameter
substitution is applied to the string before writing. The number of UTF-8 code units
successfully written is returned.
*/
size_t ZStrimW::Writef(const UTF8* iString, ...) const
	{
	string8 buffer(512, ' ');
	for (;;)
		{
		va_list args;
		va_start(args, iString);

		int count = vsnprintf(const_cast<char*>(buffer.data()), buffer.size(), iString, args);

		va_end(args);

		if (count < 0)
			{
			// Handle -ve result from glibc prior to version 2.1 by growing the string.
			buffer.resize(buffer.size() * 2);
			}
		else if (count > buffer.size())
			{
			// Handle C99 standard, where count indicates how much space would have been needed.
			buffer.resize(count);
			}
		else
			{
			// The string fitted, we can now write it out.
			buffer.resize(count);
			this->Write(buffer);
			return count;
			}
		}
	}

void ZStrimW::CopyAllFrom(const ZStrimR& iStrimR) const
	{ this->CopyAllFrom(iStrimR, nil, nil); }

void ZStrimW::CopyAllFrom(const ZStrimR& iStrimR,
	size_t* oCountCPRead, size_t* oCountCPWritten) const
	{
	if (oCountCPRead)
		*oCountCPRead = 0;

	if (oCountCPWritten)
		*oCountCPWritten = 0;

	for (;;)
		{
		size_t countRead, countWritten;
		this->CopyFrom(iStrimR, 128*1024*1024, &countRead, &countWritten);

		if (oCountCPRead)
			*oCountCPRead += countRead;

		if (oCountCPWritten)
			*oCountCPWritten += countWritten;

		if (countRead != countWritten || countRead == 0 || countWritten == 0)
			break;
		}
	}

void ZStrimW::CopyFrom(const ZStrimR& iStrimR, size_t iCountCP) const
	{
	size_t countRead, countWritten;
	this->CopyFrom(iStrimR, iCountCP, &countRead, &countWritten);
	if (countRead != iCountCP)
		ZStrimR::sThrowEndOfStrim();
	if (countWritten != iCountCP)
		sThrowEndOfStrim();
	}

void ZStrimW::CopyFrom(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten) const
	{
	const_cast<ZStrimW*>(this)->Imp_CopyFromDispatch(iStrimR,
		iCountCP, oCountCPRead, oCountCPWritten);
	}


/// If this strim buffers data then pass it on to its ultimate destination.
/**
Some write strims buffer the data written to them, or are built atop a foundation that
may buffer data. Such strims should override \c Imp_Flush and push that buffered data
on to its ultimate destination. When the method returns the data may still not have reached
its ultimate destination, but is guaranteed to do at some point in the future with
no further action necessary on the caller's part.
*/
void ZStrimW::Flush() const
	{ const_cast<ZStrimW*>(this)->Imp_Flush(); }


/// Invoke \a iStrimR's \c Imp_CopyTo method.
/**
The first part of the two-stage dispatch initiated by \c CopyFrom and \c CopyAllFrom. Override
if your strim's data is in memory and thus \a iStrimR can have its \c Read method called with
your strim's data as its destination.
*/
void ZStrimW::Imp_CopyFromDispatch(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{ const_cast<ZStrimR&>(iStrimR).Imp_CopyTo(*this, iCountCP, oCountCPRead, oCountCPWritten); }


/// Actually copy data to this strim from \a iStrimR.
/**
The second part of the two-stage dispatch initiated by \c CopyFrom and \c CopyAllFrom. The default
implementation calls \c sCopyReadToWrite. Override this method if your strim's data is in memory
and thus can be modified by calling \a iStrimR's \c Read method.
\sa sCopyReadToWrite
*/
void ZStrimW::Imp_CopyFrom(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{ sCopyReadToWrite(iStrimR, *this, iCountCP, oCountCPRead, oCountCPWritten); }


/// If this strim buffers data then pass it on to its ultimate destination.
/**
Some write strims buffer the data written to them, or are built atop a foundation that
may buffer data. Such strims should override \c Flush and push that buffered data
on to its ultimate destination. When the method returns the data may still not have reached
its ultimate destination, but is guaranteed to do at some point in the future with
no further action necessary on the caller's part.
*/
void ZStrimW::Imp_Flush()
	{}

void ZStrimW::sThrowEndOfStrim()
	{ throw ExEndOfStrim(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW::ExEndOfStrim

ZStrimW::ExEndOfStrim::ExEndOfStrim()
:	range_error("ZStrimW::ExEndOfStrim")
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_NativeUTF32

void ZStrimW_NativeUTF32::Imp_WriteUTF16(const UTF16* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF16* localSource = iSource;
	while (iCountCU)
		{
		UTF32 buffer[kBufSize];
		size_t utf16Consumed;
		size_t utf32Generated;
		if (!ZUnicode::sUTF16ToUTF32(localSource, iCountCU, &utf16Consumed,
			buffer, kBufSize, &utf32Generated))
			{
			// localSource[0] to localSource[iCountCU] ends with an incomplete UTF16 sequence.
			if (utf16Consumed == 0)
				{
				// In fact there was no valid UTF16 in localSource at all, so we'll just bail.
				break;
				}
			}

		size_t utf32Consumed;
		this->Write(buffer, utf32Generated, &utf32Consumed);
		if (utf32Consumed < utf32Generated)
			{
			// It's a truncated write. We need to convert from utf32Consumed back into
			// the number of utf16 code units.
			if (oCountCU)
				localSource += ZUnicode::sCPToCU(localSource, utf32Consumed);
			break;
			}

		localSource += utf16Consumed;
		iCountCU -= utf16Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

void ZStrimW_NativeUTF32::Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF8* localSource = iSource;
	while (iCountCU)
		{
		UTF32 buffer[kBufSize];
		size_t utf8Consumed;
		size_t utf32Generated;
		if (!ZUnicode::sUTF8ToUTF32(localSource, iCountCU, &utf8Consumed,
			buffer, kBufSize, &utf32Generated))
			{
			// localSource[0] to localSource[iCountCU] ends with an incomplete UTF8 sequence.
			if (utf8Consumed == 0)
				{
				// In fact there was no valid UTF8 in localSource at all, so it's time to bail.
				break;
				}
			}

		size_t utf32Consumed;
		this->Write(buffer, utf32Generated, &utf32Consumed);
		if (utf32Consumed < utf32Generated)
			{
			// It's a truncated write. We need to convert from utf32Consumed back into
			// the number of utf8 code units.
			if (oCountCU)
				localSource += ZUnicode::sCPToCU(localSource, utf32Consumed);
			break;
			}

		localSource += utf8Consumed;
		iCountCU -= utf8Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_NativeUTF16

void ZStrimW_NativeUTF16::Imp_WriteUTF32(const UTF32* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF32* localSource = iSource;
	while (iCountCU)
		{
		UTF16 buffer[kBufSize];

		size_t utf32Consumed;
		size_t utf16Generated;
		ZUnicode::sUTF32ToUTF16(localSource, iCountCU, &utf32Consumed,
			buffer, kBufSize, &utf16Generated, nil);

		ZAssertStop(kDebug_Strim, utf32Consumed <= iCountCU);
		ZAssertStop(kDebug_Strim, utf16Generated <= kBufSize);

		size_t utf16Consumed;
		this->Write(buffer, utf16Generated, &utf16Consumed);
		if (utf16Consumed < utf16Generated)
			{
			// It's a truncated write. We need to convert from utf16Consumed back into
			// the number of code points, which is also the number of UTF32 code units.
			if (oCountCU)
				localSource += ZUnicode::sCUToCP(buffer, utf16Consumed);
			break;
			}

		localSource += utf32Consumed;
		iCountCU -= utf32Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

void ZStrimW_NativeUTF16::Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF8* localSource = iSource;
	while (iCountCU)
		{
		UTF16 buffer[kBufSize];
		size_t utf8Consumed;
		size_t utf16Generated;
		if (!ZUnicode::sUTF8ToUTF16(localSource, iCountCU, &utf8Consumed,
			buffer, kBufSize, &utf16Generated, iCountCU, nil))
			{
			// localSource[0] to localSource[iCountCU] ends with an incomplete UTF8 sequence.
			if (utf8Consumed == 0)
				{
				// In fact there was no valid UTF8 in localSource at all, so it's time to bail.
				break;
				}
			}

		size_t utf16Consumed;
		this->Write(buffer, utf16Generated, &utf16Consumed);
		if (utf16Consumed < utf16Generated)
			{
			// It's a truncated write. We need to convert from utf16Consumed back into
			// the number of utf8 code units.
			if (oCountCU)
				{
				size_t codePoints = ZUnicode::sCUToCP(buffer, utf16Consumed);
				localSource += ZUnicode::sCPToCU(localSource, codePoints);
				}
			break;
			}

		localSource += utf8Consumed;
		iCountCU -= utf8Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_NativeUTF8

void ZStrimW_NativeUTF8::Imp_WriteUTF32(const UTF32* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF32* localSource = iSource;
	while (iCountCU)
		{
		UTF8 buffer[kBufSize];

		size_t utf32Consumed;
		size_t utf8Generated;
		ZUnicode::sUTF32ToUTF8(localSource, iCountCU, &utf32Consumed,
			buffer, kBufSize, &utf8Generated, nil);

		ZAssertStop(kDebug_Strim, utf32Consumed <= iCountCU);
		ZAssertStop(kDebug_Strim, utf8Generated <= kBufSize);

		size_t utf8Consumed;
		this->Write(buffer, utf8Generated, &utf8Consumed);
		if (utf8Consumed < utf8Generated)
			{
			// It's a truncated write. We need to convert from utf8Consumed back into
			// the number UTF32 code units..
			if (oCountCU)
				{
				size_t codePoints = ZUnicode::sCUToCP(buffer, utf8Consumed);
				localSource += ZUnicode::sCPToCU(localSource, codePoints);
				}
			break;
			}

		localSource += utf32Consumed;
		iCountCU -= utf32Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

void ZStrimW_NativeUTF8::Imp_WriteUTF16(const UTF16* iSource, size_t iCountCU, size_t* oCountCU)
	{
	const UTF16* localSource = iSource;
	while (iCountCU)
		{
		UTF8 buffer[kBufSize];
		size_t utf16Consumed;
		size_t utf8Generated;
		if (!ZUnicode::sUTF16ToUTF8(localSource, iCountCU, &utf16Consumed,
			buffer, kBufSize, &utf8Generated, iCountCU, nil))
			{
			// localSource[0] to localSource[iCountCU] ends with an incomplete UTF16 sequence.
			if (utf16Consumed == 0)
				{
				// In fact there was no valid UTF16 in localSource at all, so it's time to bail.
				break;
				}
			}

		size_t utf8Consumed;
		this->Write(buffer, utf8Generated, &utf8Consumed);
		if (utf8Consumed < utf8Generated)
			{
			// It's a truncated write. We need to convert from utf8Consumed back into
			// the number of utf16 code units.
			if (oCountCU)
				{
				size_t codePoints = ZUnicode::sCUToCP(buffer, utf8Consumed);
				localSource += ZUnicode::sCPToCU(localSource, codePoints);
				}
			break;
			}

		localSource += utf16Consumed;
		iCountCU -= utf16Consumed;
		}
	if (oCountCU)
		*oCountCU = localSource - iSource;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimR_Null

/// A derivation of ZStrimR that is empty

void ZStrimR_Null::Imp_ReadUTF32(UTF32* iDest, size_t iCount, size_t* oCount)
	{
	if (oCount)
		*oCount = 0;
	}

void ZStrimR_Null::Imp_ReadUTF16(UTF16* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	if (oCountCU)
		*oCountCU = 0;
	if (oCountCP)
		*oCountCP = 0;
	}

void ZStrimR_Null::Imp_ReadUTF8(UTF8* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	if (oCountCU)
		*oCountCU = 0;
	if (oCountCP)
		*oCountCP = 0;
	}

void ZStrimR_Null::Imp_CopyToDispatch(const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	if (oCountCPRead)
		*oCountCPRead = 0;
	if (oCountCPWritten)
		*oCountCPWritten = 0;
	}

void ZStrimR_Null::Imp_CopyTo(const ZStrimW& iStrimW,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	if (oCountCPRead)
		*oCountCPRead = 0;
	if (oCountCPWritten)
		*oCountCPWritten = 0;
	}

void ZStrimR_Null::Imp_Skip(size_t iCountCP, size_t* oCountCPSkipped)
	{
	if (oCountCPSkipped)
		*oCountCPSkipped = 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_Null

void ZStrimW_Null::Imp_WriteUTF32(const UTF32* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (oCountCU)
		*oCountCU = iCountCU;
	}

void ZStrimW_Null::Imp_WriteUTF16(const UTF16* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (oCountCU)
		*oCountCU = iCountCU;
	}

void ZStrimW_Null::Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (oCountCU)
		*oCountCU = iCountCU;
	}

void ZStrimW_Null::Imp_CopyFromDispatch(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	size_t countSkipped;
	iStrimR.Skip(iCountCP, &countSkipped);
	if (oCountCPRead)
		*oCountCPRead = countSkipped;
	if (oCountCPWritten)
		*oCountCPWritten = countSkipped;
	}

void ZStrimW_Null::Imp_CopyFrom(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	size_t countSkipped;
	iStrimR.Skip(iCountCP, &countSkipped);
	if (oCountCPRead)
		*oCountCPRead = countSkipped;
	if (oCountCPWritten)
		*oCountCPWritten = countSkipped;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimU_Unreader

ZStrimU_Unreader::ZStrimU_Unreader(const ZStrimR& iStrimSource)
:	fStrimSource(iStrimSource),
	fState(eStateFresh)
	{}

void ZStrimU_Unreader::Imp_ReadUTF32(UTF32* iDest, size_t iCount, size_t* oCount)
	{
	UTF32* localDest = iDest;
	UTF32* localDestEnd = iDest + iCount;
	while (localDest < localDestEnd)
		{
		if (fState == eStateUnread)
			{
			if (ZUnicode::sWriteInc(localDest, localDestEnd, fCP))
				fState = eStateNormal;
			}
		else
			{
			size_t countRead;
			fStrimSource.Read(localDest, localDestEnd - localDest, &countRead);
			if (countRead == 0)
				break;
			fState = eStateNormal;
			fCP = localDest[countRead - 1];
			localDest += countRead;
			}
		}

	if (oCount)
		*oCount = localDest - iDest;

	if (iCount)
		{
		if (localDest == iDest)
			{
			fState = eStateHitEnd;
			}
		else
			{
			fState = eStateNormal;
			const UTF32* temp = localDest;
			fCP = ZUnicode::sDecRead(temp);
			}
		}
	}

void ZStrimU_Unreader::Imp_ReadUTF16(UTF16* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	UTF16* localDest = iDest;
	UTF16* localDestEnd = iDest + iCountCU;
	size_t localCountCP = iCountCP;
	while (localDest + 1 < localDestEnd && localCountCP)
		{
		if (fState == eStateUnread)
			{
			if (ZUnicode::sWriteInc(localDest, localDestEnd, fCP))
				{
				--localCountCP;
				fState = eStateNormal;
				}
			}
		else
			{
			size_t countCURead;
			size_t countCPRead;
			fStrimSource.Read(localDest,
				localDestEnd - localDest, &countCURead, localCountCP, &countCPRead);

			if (countCURead == 0)
				break;
			localDest += countCURead;
			localCountCP -= countCPRead;
			}
		}

	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;

	if (iCountCP)
		{
		if (localDest == iDest)
			{
			fState = eStateHitEnd;
			}
		else
			{
			fState = eStateNormal;
			const UTF16* temp = localDest;
			fCP = ZUnicode::sDecRead(temp);
			}
		}
	}

void ZStrimU_Unreader::Imp_ReadUTF8(UTF8* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	UTF8* localDest = iDest;
	UTF8* localDestEnd = iDest + iCountCU;
	size_t localCountCP = iCountCP;
	while (localDest + 5 < localDestEnd && localCountCP)
		{
		if (fState == eStateUnread)
			{
			if (ZUnicode::sWriteInc(localDest, localDestEnd, fCP))
				{
				--localCountCP;
				fState = eStateNormal;
				}
			}
		else
			{
			size_t countCURead;
			size_t countCPRead;
			fStrimSource.Read(localDest,
				localDestEnd - localDest, &countCURead, localCountCP, &countCPRead);

			if (countCURead == 0)
				break;
			fState = eStateNormal;
			fCP = localDest[countCURead - 1];
			localDest += countCURead;
			localCountCP -= countCPRead;
			}
		}

	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;

	if (iCountCP)
		{
		if (localDest == iDest)
			{
			fState = eStateHitEnd;
			}
		else
			{
			fState = eStateNormal;
			const UTF8* temp = localDest;
			fCP = ZUnicode::sDecRead(temp);
			}
		}
	}

void ZStrimU_Unreader::Imp_Unread()
	{
	switch (fState)
		{
		case eStateFresh:
			{
			ZDebugStopf(2,
				("You called ZStrimU_Unreader::Imp_Unread without having read anything."));
			break;
			}
		case eStateNormal:
			{
			fState = eStateUnread;
			break;
			}
		case eStateUnread:
			{
			ZDebugStopf(2, ("You called ZStrimU_Unreader::Imp_Unread twice consecutively."));
			break;
			}
		case eStateHitEnd:
			{
			ZDebugStopf(2,
				("You called ZStrimU_Unreader::Imp_Unread when end of strim has been seen."));
			break;
			}
		}	
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimU_String

ZStrimU_String::ZStrimU_String(const string8& iString)
:	fString(iString),
	fPosition(0)
	{}

ZStrimU_String::~ZStrimU_String()
	{}

void ZStrimU_String::Imp_ReadUTF32(UTF32* iDest, size_t iCount, size_t* oCount)
	{
	UTF32* localDest = iDest;
	if (!fString.empty())
		{
		UTF32* localDestEnd = iDest + iCount;
		string::const_iterator localSource = fString.begin() + fPosition;
		string::const_iterator localSourceEnd = fString.end();
		for (;;)
			{
			string::const_iterator priorLocalSource = localSource;
			UTF32 theCP;
			if (!ZUnicode::sReadInc(localSource, localSourceEnd, theCP))
				break;
			if (!ZUnicode::sWriteInc(localDest, localDestEnd, theCP))
				{
				localSource = priorLocalSource;
				break;
				}
			}
		fPosition = localSource - fString.begin();
		}
	if (oCount)
		*oCount = localDest - iDest;
	}

void ZStrimU_String::Imp_ReadUTF16(UTF16* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
	size_t localCountCP = iCountCP;
	UTF16* localDest = iDest;
	if (!fString.empty())
		{
		UTF16* localDestEnd = iDest + iCountCU;
		string::const_iterator localSource = fString.begin() + fPosition;
		string::const_iterator localSourceEnd = fString.end();
		while (localCountCP)
			{
			string::const_iterator priorLocalSource = localSource;
			UTF32 theCP;
			if (!ZUnicode::sReadInc(localSource, localSourceEnd, theCP))
				break;
			if (!ZUnicode::sWriteInc(localDest, localDestEnd, theCP))
				{
				localSource = priorLocalSource;
				break;
				}
			--localCountCP;
			}
		fPosition = localSource - fString.begin();
		}
	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;
	}

void ZStrimU_String::Imp_ReadUTF8(UTF8* iDest,
	size_t iCountCU, size_t* oCountCU, size_t iCountCP, size_t* oCountCP)
	{
#if 0
	size_t localCountCP = iCountCP;
	UTF8* localDest = iDest;
	if (!fString.empty())
		{
		UTF8* localDestEnd = iDest + iCountCU;
		string::const_iterator localSource = fString.begin() + fPosition;
		string::const_iterator localSourceEnd = fString.end();
		while (localCountCP)
			{
			string::const_iterator priorLocalSource = localSource;
			UTF32 theCP;
			if (!ZUnicode::sReadInc(localSource, localSourceEnd, theCP))
				break;
			if (!ZUnicode::sWriteInc(localDest, localDestEnd, theCP))
				{
				localSource = priorLocalSource;
				break;
				}
			--localCountCP;
			}
		fPosition = localSource - fString.begin();
		}
	if (oCountCP)
		*oCountCP = iCountCP - localCountCP;
	if (oCountCU)
		*oCountCU = localDest - iDest;
#else
	if (fString.empty())
		{
		if (oCountCP)
			*oCountCP = 0;
		if (oCountCU)
			*oCountCU = 0;
		}
	else
		{
		size_t cuToCopy = min(fString.size() - fPosition, iCountCU);
		if (iCountCP < cuToCopy)
			{
			// The number of code points to transfer is less than the number of code units
			// requested and/or available, and thus may be the limit on what gets transferred.
			size_t actualCodePoints;
			size_t actualCodeUnits = ZUnicode::sCPToCU(fString.data() + fPosition,
				cuToCopy, iCountCP, &actualCodePoints);

			ZBlockCopy(fString.data() + fPosition, iDest, actualCodeUnits);
			fPosition += actualCodeUnits;
			if (oCountCP)
				*oCountCP = actualCodePoints;
			if (oCountCU)
				*oCountCU = actualCodeUnits;
			}
		else
			{
			// The number of code points to transfer is greater than or equal to the number of
			// code units requested and available, and thus cannot limit the number of code units.
			ZBlockCopy(fString.data() + fPosition, iDest, cuToCopy);
			if (oCountCP)
				*oCountCP = ZUnicode::sCUToCP(iDest, cuToCopy);
			if (oCountCU)
				*oCountCU = cuToCopy;
			}
		}
#endif
	}

void ZStrimU_String::Imp_Unread()
	{
	if (size_t stringSize = fString.size())
		{
		string::const_iterator stringStart = fString.begin();
		string::const_iterator stringCurrent = stringStart + fPosition;
		string::const_iterator stringEnd = stringStart + stringSize;

		if (ZUnicode::sDec(stringStart, stringCurrent, stringEnd))
			{
			fPosition = stringCurrent - stringStart;
			return;
			}
		}
	// Unread shouldn't have been called, as our string is empty
	// or we're already at the beginning, and thus read could not
	// have previously been called successfully.
	ZUnimplemented();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_String

ZStrimW_String::ZStrimW_String(string8& iString)
:	fString(iString)
	{}

ZStrimW_String::~ZStrimW_String()
	{}

void ZStrimW_String::Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU)
	{
	try
		{
		fString.append(iSource, iCountCU);
		if (oCountCU)
			*oCountCU = iCountCU;
		}
	catch (...)
		{}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStrimW_Indirect

ZStrimW_Indirect::ZStrimW_Indirect()
:	fSink(nil)
	{}

ZStrimW_Indirect::ZStrimW_Indirect(const ZStrimW& iStrimW)
:	fSink(&iStrimW)
	{}

ZStrimW_Indirect::~ZStrimW_Indirect()
	{}

void ZStrimW_Indirect::Imp_WriteUTF32(const UTF32* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (fSink)
		{
		fSink->Write(iSource, iCountCU, oCountCU);
		}
	else
		{
		if (oCountCU)
			*oCountCU = iCountCU;
		}
	}

void ZStrimW_Indirect::Imp_WriteUTF16(const UTF16* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (fSink)
		{
		fSink->Write(iSource, iCountCU, oCountCU);
		}
	else
		{
		if (oCountCU)
			*oCountCU = iCountCU;
		}
	}

void ZStrimW_Indirect::Imp_WriteUTF8(const UTF8* iSource, size_t iCountCU, size_t* oCountCU)
	{
	if (fSink)
		{
		fSink->Write(iSource, iCountCU, oCountCU);
		}
	else
		{
		if (oCountCU)
			*oCountCU = iCountCU;
		}
	}

void ZStrimW_Indirect::Imp_CopyFromDispatch(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	if (fSink)
		{
		fSink->CopyFrom(iStrimR, iCountCP, oCountCPRead, oCountCPWritten);
		}
	else
		{
		if (oCountCPRead)
			*oCountCPRead = iCountCP;
		if (oCountCPWritten)
			*oCountCPWritten = iCountCP;
		}
	}

void ZStrimW_Indirect::Imp_CopyFrom(const ZStrimR& iStrimR,
	size_t iCountCP, size_t* oCountCPRead, size_t* oCountCPWritten)
	{
	if (fSink)
		{
		fSink->CopyFrom(iStrimR, iCountCP, oCountCPRead, oCountCPWritten);
		}
	else
		{
		if (oCountCPRead)
			*oCountCPRead = iCountCP;
		if (oCountCPWritten)
			*oCountCPWritten = iCountCP;
		}
	}

void ZStrimW_Indirect::Imp_Flush()
	{
	if (fSink)
		fSink->Flush();
	}

void ZStrimW_Indirect::SetSink(const ZStrimW& iStrimW)
	{
	fSink = &iStrimW;
	}

void ZStrimW_Indirect::SetSink(const ZStrimW* iStrimW)
	{
	fSink = iStrimW;
	}

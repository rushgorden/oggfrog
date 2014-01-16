static const char rcsid[] = "@(#) $Id: ZStream_Buffered.cpp,v 1.8 2006/07/23 21:58:20 agreen Exp $";

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

#include "ZStream_Buffered.h"
#include "ZMemory.h" // For ZBlockMove & ZBlockCopy

using std::min;

#define kDebug_Stream_Buffered 2

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_Buffered

/**
\class ZStreamR_Buffered
\ingroup stream
ZStreamR_Buffered filters its source stream by trying to read in chunks that are as large as the
buffer size passed to its constructor. If the source stream's CountReadable method always returns
zero then ZStreamR_Buffered will not be able to help out -- it relies on a usable answer from that
method to read more data than is required to satisfy the current request, in order to be able to
satisfy future requests from its buffer rather than by calling the source stream again.
ZStreamR_Buffered is most useful to ameliorate poor performance caused by a source stream with
high call overhead or latency.
*/

ZStreamR_Buffered::ZStreamR_Buffered(size_t iBufferSize, const ZStreamR& iStreamSource)
:	fStreamSource(iStreamSource)
	{
	// Ensure we have a reasonable size of buffer
	if (iBufferSize < 128)
		iBufferSize = 128;
	else if (iBufferSize > 8192)
		iBufferSize = 8192;
	fBufferSize = iBufferSize;
	fBuffer = new uint8[fBufferSize];
	fBufferOffset = fBufferSize;
	}

ZStreamR_Buffered::~ZStreamR_Buffered()
	{
// This check causes more trouble than its worth.
//	if (fBufferSize - fBufferOffset)
//		ZDebugLogf(1, ("~ZStreamR_Buffered::~~ZStreamR_Buffered, disposed with unread data"));
	delete[] fBuffer;
	}

void ZStreamR_Buffered::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	uint8* localDest = reinterpret_cast<uint8*>(iDest);
	while (iCount)
		{
		size_t countInBuffer = fBufferSize - fBufferOffset;
		if (countInBuffer)
			{
			// We have some data in our buffer, use it up first.
			size_t countToMove = min(countInBuffer, iCount);
			ZBlockCopy(fBuffer + fBufferOffset, localDest, countToMove);
			fBufferOffset += countToMove;
			localDest += countToMove;
			iCount -= countToMove;
			}
		else
			{
			// Our buffer is empty.
			size_t countReadable = fStreamSource.CountReadable();
			if (iCount >= fBufferSize || iCount >= countReadable)
				{
				// Either we're asking for more data than would fit in our buffer, or we're asking
				// for more data than the stream will be able to provide without blocking. In
				// either case we bypass the buffer and read straight into our read destination
				size_t countRead;
				fStreamSource.Read(localDest, iCount, &countRead);
				if (countRead == 0)
					break;
				localDest += countRead;
				iCount -= countRead;
				}
			else
				{
				// We're asking for less data than the stream guarantees it could provide without
				// blocking, in which case we fill up as much of our buffer as we can,  so some
				// later request will be able to be satisfied straight from our buffer.
				size_t countToRead = min(fBufferSize, countReadable);
				size_t countRead;
				fStreamSource.Read(fBuffer + (fBufferSize - countToRead), countToRead, &countRead);
				if (countRead == 0)
					break;
				// If countRead is less than what we asked for, shift the buffer contents so that
				// the last byte read aligns with the last byte of the buffer, otherwise we need
				// to maintain an additional instance variable to track not just where we're
				// feeding out from (fBufferOffset), but just how much more data is actually valid.
				if (countRead < countToRead)
					{
					ZBlockMove(fBuffer + (fBufferSize - countToRead),
						fBuffer + (fBufferSize - countRead), countRead);
					}
				fBufferOffset = fBufferSize - countRead;
				}
			}
		}
	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<uint8*>(iDest);
	}

size_t ZStreamR_Buffered::Imp_CountReadable()
	{ return fBufferSize - fBufferOffset + fStreamSource.CountReadable(); }

void ZStreamR_Buffered::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	if (oCountSkipped)
		*oCountSkipped = 0;

	// Skip as much as we might have buffered
	if (uint64 countToSkip = min(uint64(fBufferSize - fBufferOffset), iCount))
		{
		iCount -= countToSkip;
		fBufferOffset += countToSkip;
		if (oCountSkipped)
			*oCountSkipped += countToSkip;
		}

	// If there's more to skip, pass it on to our source stream
	if (iCount)
		{
		uint64 countSkipped;
		fStreamSource.Skip(iCount, &countSkipped);
		if (oCountSkipped)
			*oCountSkipped += countSkipped;
		}
	}

void ZStreamR_Buffered::Abandon()
	{
	fBufferOffset = fBufferSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamW_Buffered

/**
\class ZStreamW_Buffered
\ingroup stream
ZStreamW_Buffered buffers data written to it. When its buffer fills it writes the entire buffer to
its sink stream. It also writes any data in its buffer when Flush is called. If a caller writes
more data than the buffer can hold, pending data is written to the sink stream and the new data is
passed straight through to the sink stream, bypassing the buffer altogether.
*/

ZStreamW_Buffered::ZStreamW_Buffered(size_t iBufferSize, const ZStreamW& iStreamSink)
:	fStreamSink(iStreamSink)
	{
	// Ensure we have a reasonable size of buffer
	if (iBufferSize < 128)
		iBufferSize = 128;
	else if (iBufferSize > 8192)
		iBufferSize = 8192;
	fBufferSize = iBufferSize;
	fBuffer = new uint8[fBufferSize];
	fBufferOffset = 0;
	}

ZStreamW_Buffered::~ZStreamW_Buffered()
	{
	if (fBufferOffset != 0)
		{
		size_t countWritten;
		fStreamSink.Write(fBuffer, fBufferOffset, &countWritten);
		if (countWritten != fBufferOffset)
			ZDebugLogf(1, ("ZStreamW_Buffered::~ZStreamW_Buffered, unable to flush entire buffer"));
		}
	delete[] fBuffer;
	}

void ZStreamW_Buffered::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	const uint8* localSource = reinterpret_cast<const uint8*>(iSource);
	while (iCount)
		{
		if (fBufferOffset == 0 && fBufferSize <= iCount)
			{
			// We have an empty buffer *and* we have more data to send than would fit in the buffer.
			size_t countWritten;
			fStreamSink.Write(localSource, iCount, &countWritten);
			if (countWritten == 0)
				break;
			localSource += countWritten;
			iCount -= countWritten;
			}
		else
			{
			// Either we already have data in the buffer, or we have an empty buffer
			// and less than a buffer's worth to send.
			size_t countToCopy = min(iCount, fBufferSize - fBufferOffset);
			ZBlockCopy(localSource, fBuffer + fBufferOffset, countToCopy);
			fBufferOffset += countToCopy;
			localSource += countToCopy;
			iCount -= countToCopy;
			if (fBufferOffset == fBufferSize)
				{
				// We've filled the buffer, so try to empty it, and stop consuming data
				// from our source. At some point we'll be called again and be unable to
				// consume any data at all (if we even managed to this time), and our
				// caller will know there's something wrong.
				if (!this->Internal_Flush())
					break;
				}
			}
		}
	if (oCountWritten)
		*oCountWritten = localSource - reinterpret_cast<const uint8*>(iSource);
	}

void ZStreamW_Buffered::Imp_Flush()
	{
	if (fBufferOffset != 0)
		this->Internal_Flush();

	fStreamSink.Flush();
	}

void ZStreamW_Buffered::Abandon()
	{
	fBufferOffset = 0;
	}

bool ZStreamW_Buffered::Internal_Flush()
	{
	size_t countRemaining = fBufferOffset;
	const uint8* localSource = reinterpret_cast<const uint8*>(fBuffer);
	while (countRemaining > 0)
		{
		size_t countWritten;
		fStreamSink.Write(localSource, countRemaining, &countWritten);
		if (countWritten == 0)
			return false;
		localSource += countWritten;
		countRemaining -= countWritten;
		}

	fBufferOffset = 0;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamR_DynamicBuffered

/**
\class ZStreamR_DynamicBuffered
\ingroup stream
ZStreamR_DynamicBuffered is most useful when you have a ZStreamR containing some data that you'd
like to be able to read repeatedly, followed by other data that will not need to be re-read. A good
example is a network stream with pixmap data on it in some format. We want to be able to pass the
first few bytes of it to multiple methods to 'sniff' its format, and then pass all the data,
including anything that got sniffed, to whichever method can handle the detected format. Even if
we're not sniffing we may need to read some of the header of some compressed image data to
determine what size pixmap should be allocated, and then pass the entire data to some external
code that will do the decompression.

A ZStreamR_DynamicBuffered is in appending mode or non-appending mode. When it's appending,
as it is when first instantiated, reads are first satisfied from the ZStreamRWPos, and then
from the ZStreamR. Anything read from the ZStreamR is appended to the ZStreamRWPos. Calling
\a Rewind causes subsequent reads to again be fulfilled starting from the beginning of the
ZStreamRWPos, until it's again exhausted when data will be fetched from the ZStreamR
and appended to the ZStreamRWPos.

To switch from appending mode to non-appending mode call Commit. In non-appending mode data
will still initially be satisfied from the ZStreamRWPos, but once exhausted the ZStreamRWPos
will be resized to zero bytes and all subsequent reads will be satsified from the ZStreamR.
Note that it is an error to call \a Commit more than once, or to call \a Rewind
after \a Commit has been called.

For example, assuming we're passed \a iStreamR containing some image data, and we want to
use (the hypothetical) SniffFormat to detect what format it's in before calling
(the hypothetical) DecompressFormat:

\code
ZStreamRWPos_RAM theBuffer;
ZStreamR_DynamicBuffered theStreamRDB(iStreamR, theBuffer);
for (Format theFormat = firstFormat; theFormat != endFormat; ++theFormat)
	{
	if (SniffFormat(theStreamRDB, theFormat))
		{
		// Return the read data so it's all available to DecompressFormat.
		theStreamRDB.Rewind();

		// We won't need to re-read the data again, and don't
		// want the entire rest of iStreamR's data to be copied into
		// theBuffer.
		theStreamRDB.Commit();
		DecompressFormat(theStreamRDB, theFormat);
		break;
		}
	// We rewind theStreamRDB so that the next call to SniffFormat will
	// see the same contents as this call, possibly pulling a little extra
	// data off iStreamR, or possibly not. theBuffer ends up reaching a
	// high water mark, holding all the data up to the largest amount that
	// SniffFormat ever needed to read to determine a format.
	theStreamRDB.Rewind();
	}
/endcode
*/

ZStreamR_DynamicBuffered::ZStreamR_DynamicBuffered(
	const ZStreamR& iStreamSource, const ZStreamRWPos& iStreamBuffer)
:	fStreamSource(iStreamSource),
	fStreamBuffer(const_cast<ZStreamRWPos&>(iStreamBuffer)),
	fMode(eModeReadBufferThenAppend)
	{}

ZStreamR_DynamicBuffered::~ZStreamR_DynamicBuffered()
	{}

void ZStreamR_DynamicBuffered::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	char* localDest = static_cast<char*>(iDest);
	while (iCount)
		{
		size_t countRead;
		if (fMode == eModeReadBufferThenAppend || fMode == eModeReadBufferThenStream)
			{
			fStreamBuffer.Read(localDest, iCount, &countRead);
			if (countRead == 0)
				{
				if (fMode == eModeReadBufferThenAppend)
					{
					fMode = eModeAppend;
					}
				else
					{
					// We can discard our buffer's contents.
					fStreamBuffer.SetSize(0);
					fMode = eModeStream;
					}
				}
			}
		else if (fMode == eModeAppend)
			{
			// We have to read into a local buffer because we're going to pass
			// what we read to fStreamBuffer, and iDest could reference memory that's
			// not safe to read (the garbage buffer, for example).
			char buffer[ZooLib::sStackBufferSize];
			fStreamSource.Read(buffer, min(iCount, sizeof(buffer)), &countRead);
			if (countRead == 0)
				break;
			ZBlockCopy(buffer, localDest, countRead);
	
			size_t countWritten;
			fStreamBuffer.Write(buffer, countRead, &countWritten);
			// This is another case where we're basically screwed if the
			// write stream goes dead on us, as far as preserving ZStreamR
			// semantics and not reading more data than we can dispose of.
			ZAssertLog(kDebug_Stream_Buffered, countRead == countWritten);	
			}
		else
			{
			fStreamSource.Read(localDest, iCount, &countRead);
			if (countRead == 0)
				break;
			}

		localDest += countRead;
		iCount -= countRead;
		}

	if (oCountRead)
		*oCountRead = localDest - static_cast<char*>(iDest);
	}

size_t ZStreamR_DynamicBuffered::Imp_CountReadable()
	{
	if (fMode == eModeReadBufferThenAppend || fMode == eModeReadBufferThenStream)
		return fStreamBuffer.CountReadable();
	else
		return fStreamSource.CountReadable();
	}

void ZStreamR_DynamicBuffered::Rewind()
	{
	// We can only rewind if Commit has not been called.
	ZAssertStop(kDebug_Stream_Buffered, fMode == eModeReadBufferThenAppend || fMode == eModeAppend);
	fStreamBuffer.SetPosition(0);
	fMode = eModeReadBufferThenAppend;
	}

void ZStreamR_DynamicBuffered::Commit()
	{
	// We can only commit if Commit has not been called.
	ZAssertStop(kDebug_Stream_Buffered, fMode == eModeReadBufferThenAppend || fMode == eModeAppend);
	if (fMode == eModeReadBufferThenAppend)
		{
		// We can't discard our buffer's contents yet. That will happen when
		// we have finished reading everything from it in Imp_Read.
		fMode = eModeReadBufferThenStream;
		}
	else
		{
		// We can discard our buffer's contents.
		fStreamBuffer.SetSize(0);
		fStreamBuffer.SetPosition(0);
		fMode = eModeStream;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerR_DynamicBuffered

/// A read filter streamer encapsulating a ZStreamR_DynamicBuffered.

ZStreamerR_DynamicBuffered::ZStreamerR_DynamicBuffered(
	ZRef<ZStreamerR> iStreamerSource, ZRef<ZStreamerRWPos> iStreamerBuffer)
:	fStreamerSource(iStreamerSource),
	fStreamerBuffer(iStreamerBuffer),
	fStream(iStreamerSource->GetStreamR(), iStreamerBuffer->GetStreamRWPos())
	{}

ZStreamerR_DynamicBuffered::~ZStreamerR_DynamicBuffered()
	{}

const ZStreamR& ZStreamerR_DynamicBuffered::GetStreamR()
	{ return fStream; }

ZStreamR_DynamicBuffered& ZStreamerR_DynamicBuffered::GetStream()
	{ return fStream; }

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamW_DynamicBuffered

/**
\class ZStreamW_DynamicBuffered
\ingroup stream
ZStreamW_DynamicBuffered is a filter that buffers all writes made against it by writing that data
into the ZStreamRWPos passed to its constructor. When Flush is called or the stream is disposed
the accumulated data is copied from the buffering ZStreamRWPos to the sink write stream, the
ZStreamRWPos is then resized back to be empty and all subsequent writes are again accumulated until
the next flush, or until disposal.
*/

ZStreamW_DynamicBuffered::ZStreamW_DynamicBuffered(
	const ZStreamW& iStreamSink, const ZStreamRWPos& iStreamBuffer)
:	fStreamSink(iStreamSink),
	fStreamBuffer(const_cast<ZStreamRWPos&>(iStreamBuffer))
	{}

ZStreamW_DynamicBuffered::~ZStreamW_DynamicBuffered()
	{
	try
		{
		if (uint64 countToWrite = fStreamBuffer.GetSize())
			{
			fStreamBuffer.SetPosition(0);
			fStreamSink.CopyFrom(fStreamBuffer, countToWrite);
			fStreamBuffer.SetPosition(0);
			fStreamBuffer.Truncate();
			}
		}
	catch (...)
		{}
	}

void ZStreamW_DynamicBuffered::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{ fStreamBuffer.Write(iSource, iCount, oCountWritten); }

void ZStreamW_DynamicBuffered::Imp_CopyFromDispatch(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{ fStreamBuffer.CopyFrom(iStreamR, iCount, oCountRead, oCountWritten); }

void ZStreamW_DynamicBuffered::Imp_CopyFrom(const ZStreamR& iStreamR, uint64 iCount,
	uint64* oCountRead, uint64* oCountWritten)
	{ fStreamBuffer.CopyFrom(iStreamR, iCount, oCountRead, oCountWritten); }

void ZStreamW_DynamicBuffered::Imp_Flush()
	{
	if (uint64 countToWrite = fStreamBuffer.GetSize())
		{
		fStreamBuffer.SetPosition(0);
		fStreamBuffer.CopyTo(fStreamSink, countToWrite);
		fStreamBuffer.SetPosition(0);
		fStreamBuffer.Truncate();
		}
	fStreamSink.Flush();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamerW_DynamicBuffered

ZStreamerW_DynamicBuffered::ZStreamerW_DynamicBuffered(
	ZRef<ZStreamerW> iStreamerSink, ZRef<ZStreamerRWPos> iStreamerBuffer)
:	fStreamerSink(iStreamerSink),
	fStreamerBuffer(iStreamerBuffer),
	fStream(iStreamerSink->GetStreamW(), iStreamerBuffer->GetStreamRWPos())
	{}

ZStreamerW_DynamicBuffered::~ZStreamerW_DynamicBuffered()
	{}

const ZStreamW& ZStreamerW_DynamicBuffered::GetStreamW()
	{ return fStream; }

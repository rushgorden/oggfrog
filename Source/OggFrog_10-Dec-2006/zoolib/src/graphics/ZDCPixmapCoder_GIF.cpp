static const char rcsid[]
= "@(#) $Id: ZDCPixmapCoder_GIF.cpp,v 1.17 2006/04/26 22:31:48 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZDCPixmapCoder_GIF.h"
#include "ZMemory.h"
#include "ZStream_LZW.h"

using std::min;
using std::runtime_error;
using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * StreamW_Chunk

namespace ZANONYMOUS {

class StreamW_Chunk : public ZStreamW
	{
public:
	StreamW_Chunk(const ZStreamW& iStreamSink);
	~StreamW_Chunk();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);
	virtual void Imp_Flush();

private:
	void Internal_Flush();

	const ZStreamW& fStreamSink;
	uint8* fBuffer;
	size_t fBufferUsed;
	};

StreamW_Chunk::StreamW_Chunk(const ZStreamW& iStreamSink)
:	fStreamSink(iStreamSink)
	{
	fBuffer = new uint8[256];
	fBufferUsed = 0;
	}

StreamW_Chunk::~StreamW_Chunk()
	{
	try
		{
		this->Internal_Flush();
		fStreamSink.WriteUInt8(0); // Terminating zero-length chunk.
		}
	catch (...)
		{}

	delete[] fBuffer;
	}

void StreamW_Chunk::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	const uint8* localSource = reinterpret_cast<const uint8*>(iSource);
	while (iCount)
		{
		if (fBufferUsed == 255)
			this->Internal_Flush();
		size_t countToCopy = min(iCount, size_t(255 - fBufferUsed));
		ZBlockCopy(localSource, fBuffer + fBufferUsed, countToCopy);
		fBufferUsed += countToCopy;
		iCount -= countToCopy;
		localSource += countToCopy;
		}
	if (oCountWritten)
		*oCountWritten = localSource - reinterpret_cast<const uint8*>(iSource);
	}

void StreamW_Chunk::Imp_Flush()
	{
	this->Internal_Flush();
	fStreamSink.Flush();
	}

void StreamW_Chunk::Internal_Flush()
	{
	if (fBufferUsed)
		{
		fStreamSink.WriteUInt8(fBufferUsed);
		size_t countWritten;
		fStreamSink.Write(fBuffer, fBufferUsed, &countWritten);
		fBufferUsed = 0;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * StreamR_Chunk

class StreamR_Chunk : public ZStreamR
	{
public:
	StreamR_Chunk(const ZStreamR& iStreamSource);
	~StreamR_Chunk();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();
	virtual void Imp_Skip(uint64 iCount, uint64* oCountSkipped);

private:
	const ZStreamR& fStreamSource;
	size_t fChunkSize;
	bool fHitEnd;
	};

StreamR_Chunk::StreamR_Chunk(const ZStreamR& iStreamSource)
:	fStreamSource(iStreamSource)
	{
	fChunkSize = fStreamSource.ReadUInt8();
	fHitEnd = fChunkSize == 0;
	}

StreamR_Chunk::~StreamR_Chunk()
	{}

void StreamR_Chunk::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	uint8* localDest = reinterpret_cast<uint8*>(iDest);
	while (iCount && !fHitEnd)
		{
		if (fChunkSize == 0)
			{
			fChunkSize = fStreamSource.ReadUInt8();
			if (fChunkSize == 0)
				fHitEnd = true;
			}
		else
			{
			size_t countRead;
			fStreamSource.Read(localDest, min(iCount, fChunkSize), &countRead);
			localDest += countRead;
			iCount -= countRead;
			fChunkSize -= countRead;
			}
		}
	if (oCountRead)
		*oCountRead = localDest - reinterpret_cast<uint8*>(iDest);
	}

size_t StreamR_Chunk::Imp_CountReadable()
	{ return min(fChunkSize, fStreamSource.CountReadable()); }

void StreamR_Chunk::Imp_Skip(uint64 iCount, uint64* oCountSkipped)
	{
	uint64 countRemaining = iCount;
	while (countRemaining && !fHitEnd)
		{
		if (fChunkSize == 0)
			{
			fChunkSize = fStreamSource.ReadUInt8();
			if (fChunkSize == 0)
				fHitEnd = true;
			}
		else
			{
			uint64 countSkipped;
			fStreamSource.Skip(min(countRemaining, uint64(fChunkSize)), &countSkipped);
			countRemaining -= countSkipped;
			fChunkSize -= countSkipped;
			}
		}

	if (oCountSkipped)
		*oCountSkipped = iCount - countRemaining;
	}

} // anonymous namespace

static const int sInterlaceStart[] = { 0, 4, 2, 1 };
static const int sInterlaceIncrement[] = { 8, 8, 4, 2 };

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapEncoder_GIF

void ZDCPixmapEncoder_GIF::sWritePixmap(const ZStreamW& iStream, const ZDCPixmap& iPixmap)
	{
	ZDCPixmapEncoder_GIF theEncoder(false);
	ZDCPixmapEncoder::sWritePixmap(iStream, iPixmap, theEncoder); 
	}

//! Regular constructor, \param iInterlace should be true to generate an interlaced GIF.
ZDCPixmapEncoder_GIF::ZDCPixmapEncoder_GIF(bool iInterlace)
	{
	fInterlace = iInterlace;
	fNoPatent = false;
	fTransparent = false;
	}

/*!
\param iInterlace should be true to generate an interlaced GIF.
\param iNoPatent should be true to generate a GIF without infringing the Unisys patent.
*/
ZDCPixmapEncoder_GIF::ZDCPixmapEncoder_GIF(bool iInterlace, bool iNoPatent)
	{
	fInterlace = iInterlace;
	fNoPatent = iNoPatent;
	fTransparent = false;
	}

/*!
\param iInterlace should be true to generate an interlaced GIF.
\param iNoPatent should be true to generate a GIF without infringing the Unisys patent.
*/
ZDCPixmapEncoder_GIF::ZDCPixmapEncoder_GIF(
	bool iInterlace, bool iNoPatent, uint32 iTransparentPixval)
	{
	fInterlace = iInterlace;
	fNoPatent = iNoPatent;
	fTransparentPixval = iTransparentPixval;
	fTransparent = true;
	}

ZDCPixmapEncoder_GIF::~ZDCPixmapEncoder_GIF()
	{}

static void sWriteColorTable(const ZStreamW& iStream,
	const ZRGBColorPOD* iColors, size_t iCountAvailable, size_t iCountNeeded)
	{
	iCountAvailable = min(iCountAvailable, iCountNeeded);

	vector<uint8> tempColorsVector(iCountNeeded * 3);
	uint8* currentDest = &tempColorsVector[0];

	for (size_t x = 0; x < iCountAvailable; ++x)
		{
		*currentDest++ = iColors->red >> 8;
		*currentDest++ = iColors->green >> 8;
		*currentDest++ = iColors->blue >> 8;
		++iColors;
		}

	for (size_t x = iCountAvailable; x < iCountNeeded; ++x)
		{
		*currentDest++ = 0;
		*currentDest++ = 0;
		*currentDest++ = 0;
		}

	iStream.Write(&tempColorsVector[0], tempColorsVector.size());
	}

void ZDCPixmapEncoder_GIF::Imp_Write(const ZStreamW& iStream,
	const void* iBaseAddress,
	const ZDCPixmapNS::RasterDesc& iRasterDesc,
	const ZDCPixmapNS::PixelDesc& iPixelDesc,
	const ZRect& iBounds)
	{
	ZRef<ZDCPixmapNS::PixelDescRep_Indexed> thePixelDescRep_Indexed
		= ZRefDynamicCast<ZDCPixmapNS::PixelDescRep_Indexed>(iPixelDesc.GetRep());
	ZAssertStop(2, thePixelDescRep_Indexed);

	if (fTransparent)
		iStream.WriteString("GIF89a");
	else
		iStream.WriteString("GIF87a");
	iStream.WriteUInt16LE(iBounds.Width());
	iStream.WriteUInt16LE(iBounds.Height());

	uint8 globalStrmFlags = 0;
	globalStrmFlags |= 0x80; // hasGlobalColorTable
	globalStrmFlags |= 0x70; // colorResolution (8 bits per component)
	// globalStrmFlags |= 0x08; // set this if the color table is sorted in priority order

	ZAssertStop(2, iRasterDesc.fPixvalDesc.fDepth > 0 && iRasterDesc.fPixvalDesc.fDepth <= 8);
	
	globalStrmFlags |= iRasterDesc.fPixvalDesc.fDepth - 1; // globalColorTableSize & depth
	iStream.WriteUInt8(globalStrmFlags);

	iStream.WriteUInt8(0); // backgroundColorIndex
	iStream.WriteUInt8(0); // Pixel aspect ratio -- 0 == none specified.

	const ZRGBColorPOD* theColors;
	size_t theColorsCount;
	thePixelDescRep_Indexed->GetColors(theColors, theColorsCount);
	sWriteColorTable(iStream, theColors, theColorsCount, 1 << iRasterDesc.fPixvalDesc.fDepth);

	if (fTransparent)
		{
		iStream.WriteUInt8('!'); // Extension block
		iStream.WriteUInt8(0xF9); // Graphic Control Extension
		iStream.WriteUInt8(4); // Block size
		// The next byte encodes four fields:
		// 3 bits, Reserved == 0
		// 3 bits, Disposal Method == none (0),
		// 1 bit, User Input Flag == none (0)
		// 1 bit, Transparent Color Flag = yes (1)
		iStream.WriteUInt8(1);
		iStream.WriteUInt16LE(0); // Delay time
		iStream.WriteUInt8(fTransparentPixval);
		iStream.WriteUInt8(0); // Block terminator
		}

	iStream.WriteUInt8(','); // Start of image
	iStream.WriteUInt16LE(0); // Origin h
	iStream.WriteUInt16LE(0); // Origin v
	iStream.WriteUInt16LE(iBounds.Width());
	iStream.WriteUInt16LE(iBounds.Height());

	uint8 localStrmFlags = 0;
//	localStrmFlags |= 0x80; // hasLocalColorTable
	if (fInterlace)
		localStrmFlags |= 0x40; // interlaced
//	localStrmFlags |= 0x20; // sorted
//	localStrmFlags |= 0x70; // localColorTableSize
	iStream.WriteUInt8(localStrmFlags);

	iStream.WriteUInt8(iRasterDesc.fPixvalDesc.fDepth); // Initial code size.

	{ // Scope theSC.
	StreamW_Chunk theSC(iStream);

	ZStreamW_LZWEncode* theSLZW = nil;
	ZStreamW_LZWEncodeNoPatent* theSLZWNP = nil;

	ZStreamW* theStream;
	if (fNoPatent)
		{
		theSLZWNP = new ZStreamW_LZWEncodeNoPatent(iRasterDesc.fPixvalDesc.fDepth, theSC);
		theStream = theSLZWNP;
		}
	else
		{
		theSLZW = new ZStreamW_LZWEncode(iRasterDesc.fPixvalDesc.fDepth, theSC);
		theStream = theSLZW;
		}

	ZDCPixmapNS::PixvalDesc destPixvalDesc(8, true);

	vector<uint8> theRowBufferVector(iBounds.Width());
	void* theRowBuffer = &theRowBufferVector[0];

	try
		{
		if (fInterlace)
			{
			for (int pass = 0; pass < 4; ++pass)
				{
				for (ZCoord currentY = iBounds.top + sInterlaceStart[pass];
					currentY < iBounds.bottom; currentY += sInterlaceIncrement[pass])
					{
					const void* sourceRowAddress
						= iRasterDesc.CalcRowAddress(iBaseAddress, currentY);

					ZDCPixmapNS::sBlitRowPixvals(
						sourceRowAddress, iRasterDesc.fPixvalDesc, iBounds.left,
						theRowBuffer, destPixvalDesc, 0,
						iBounds.Width());

					theStream->Write(theRowBuffer, iBounds.Width());
					}
				}
			}
		else
			{
			for (ZCoord currentY = iBounds.top; currentY < iBounds.bottom; ++currentY)
				{
				const void* sourceRowAddress
					= iRasterDesc.CalcRowAddress(iBaseAddress, currentY);

				ZDCPixmapNS::sBlitRowPixvals(
					sourceRowAddress, iRasterDesc.fPixvalDesc, iBounds.left,
					theRowBuffer, destPixvalDesc, 0,
					iBounds.Width());
				theStream->Write(theRowBuffer, iBounds.Width());
				}
			}
		}
	catch (...)
		{
		delete theSLZW;
		delete theSLZWNP;
		throw;
		}

	delete theSLZW;
	delete theSLZWNP;
	}

	iStream.WriteUInt8(';'); // Trailer.
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapDecoder_GIF

static void sReadColorTable(const ZStreamR& iStream,
	size_t iCount, vector<ZRGBColorPOD>& oColorTable)
	{
	oColorTable.resize(iCount);

	vector<uint8> readColorTable(iCount * 3);
	iStream.Read(&readColorTable[0], readColorTable.size());

	const uint8* readColor = &readColorTable[0];
	ZRGBColorPOD* oColor = &oColorTable[0];
	for (size_t x = 0; x < iCount; ++x)
		{
		oColor->red = (*readColor++) * 0x101;
		oColor->green = (*readColor++) * 0x101;
		oColor->blue = (*readColor++) * 0x101;
		oColor->alpha = 0xFFFFU;
		++oColor;
		}
	}

static void sReadImageData(const ZStreamR& iStream,
	bool iInterlaced, const ZRect& iBounds, ZRef<ZDCPixmapRaster> ioRaster)
	{
	uint8 initialCodeSize = iStream.ReadUInt8();

	StreamR_Chunk theSIC(iStream);
	ZStreamR_LZWDecode theSILZW(initialCodeSize, theSIC);

	ZDCPixmapNS::PixvalDesc sourcePixvalDesc(8, true);

	void* destBaseAddress = ioRaster->GetBaseAddress();

	ZDCPixmapNS::RasterDesc destRasterDesc = ioRaster->GetRasterDesc();

	vector<uint8> theRowBufferVector(iBounds.Width());
	void* theRowBuffer = &theRowBufferVector[0];

	if (iInterlaced)
		{
		for (int pass = 0; pass < 4; ++pass)
			{
			for (ZCoord currentY = iBounds.top + sInterlaceStart[pass];
				currentY < iBounds.bottom; currentY += sInterlaceIncrement[pass])
				{
				theSILZW.Read(theRowBuffer, iBounds.Width());
				void* destRowAddress = destRasterDesc.CalcRowAddress(destBaseAddress, currentY);

				ZDCPixmapNS::sBlitRowPixvals(theRowBuffer, sourcePixvalDesc, 0,
					destRowAddress, destRasterDesc.fPixvalDesc, iBounds.left,
					iBounds.Width());
				}
			}
		}
	else
		{
		for (ZCoord currentY = iBounds.top; currentY < iBounds.bottom; ++currentY)
			{
			theSILZW.Read(theRowBuffer, iBounds.Width());
			void* destRowAddress = destRasterDesc.CalcRowAddress(destBaseAddress, currentY);

			ZDCPixmapNS::sBlitRowPixvals(theRowBuffer, sourcePixvalDesc, 0,
				destRowAddress, destRasterDesc.fPixvalDesc, iBounds.left,
				iBounds.Width());
			}
		}
	}

ZDCPixmap ZDCPixmapDecoder_GIF::sReadPixmap(const ZStreamR& iStream)
	{
	ZDCPixmapDecoder_GIF theDecoder;
	return ZDCPixmapDecoder::sReadPixmap(iStream, theDecoder);
	}

ZDCPixmapDecoder_GIF::ZDCPixmapDecoder_GIF()
:	fReadHeader(false),
	fReadEnd(false)
	{}

ZDCPixmapDecoder_GIF::~ZDCPixmapDecoder_GIF()
	{}

static void sThrowBadFormat()
	{
	throw runtime_error("ZDCPixmapDecoder_GIF::Imp_Read, invalid BMP file");
	}

void ZDCPixmapDecoder_GIF::Imp_Read(const ZStreamR& iStream, ZDCPixmap& oPixmap)
	{
	oPixmap = ZDCPixmap();
	if (fReadEnd)
		return;

	if (!fReadHeader)
		{
		fReadHeader = true;
		if ('G' != iStream.ReadUInt8()
			|| 'I' != iStream.ReadUInt8()
			|| 'F' != iStream.ReadUInt8()
			|| '8' != iStream.ReadUInt8())
			{
			sThrowBadFormat();
			}

		uint8 version = iStream.ReadUInt8();
		if ('7' != version && '9' != version)
			sThrowBadFormat();

		if ('a' != iStream.ReadUInt8())
			sThrowBadFormat();

		fIs89a = version == '9';
	
		fSize.h = iStream.ReadUInt16LE();
		fSize.v = iStream.ReadUInt16LE();
	
		uint8 strmFlags = iStream.ReadUInt8();
			bool strmHasGlobalColorTable = (strmFlags & 0x80) != 0;
			uint8 strmColorResolution = (strmFlags & 0x7) >> 4;
			bool strmSortFlag = (strmFlags & 0x08) != 0;
			uint8 strmGlobalColorTableSize = (strmFlags & 0x7);
	
		uint8 strmBackgroundColorIndex = iStream.ReadUInt8();
		uint8 strmPixelAspectRatio = iStream.ReadUInt8();
	
		if (strmHasGlobalColorTable)
			{
			vector<ZRGBColorPOD> theColors;
			sReadColorTable(iStream, 1 << (strmGlobalColorTableSize + 1), theColors);
			fPixelDesc = ZDCPixmapNS::PixelDesc(&theColors[0], theColors.size());
			}

		static int sPhysicalDepths[] =
			{
			1, // 0
			2, // 1
			4, // 2
			4, // 3
			8, // 4
			8, // 5
			8, // 6
			8, // 7
			};
		int rowBytes
			= ZDCPixmapNS::sCalcRowBytes(sPhysicalDepths[strmGlobalColorTableSize], fSize.h);

		ZDCPixmapNS::RasterDesc theRasterDesc(
			ZDCPixmapNS::PixvalDesc(strmGlobalColorTableSize + 1, true),
			rowBytes, fSize.v, false);

		fRaster = new ZDCPixmapRaster_Simple(theRasterDesc);
		fRaster->Fill(strmBackgroundColorIndex);
		}
	else
		{
		// Clone our existing raster which contains the pixels
		// making up the pixmap we last returned.
		fRaster = new ZDCPixmapRaster_Simple(fRaster);
		}

	for (;;)
		{
		uint8 blockType = iStream.ReadUInt8();
		if (blockType == ';')
			{
			// We've reached the end of the GIF stream.
			fReadEnd = true;
			return;
			}
		else if (blockType == '!')
			{
			// It's an extension.

			// Ignore the extension type
			iStream.ReadUInt8();

			// Skip any data blocks.
			StreamR_Chunk(iStream).SkipAll();
			}
		else if (blockType  == ',')
			{
			// It's an image.
			ZRect curBounds;
			curBounds.left = iStream.ReadUInt16LE();
			curBounds.top = iStream.ReadUInt16LE();
			curBounds.right = curBounds.left + iStream.ReadUInt16LE();
			curBounds.bottom = curBounds.top + iStream.ReadUInt16LE();

			uint8 strmFlags = iStream.ReadUInt8();
				bool strmHasLocalColorTable = (strmFlags & 0x80) != 0;
				bool strmIsInterlaced = (strmFlags & 0x40) != 0;
				bool strmSortFlag = (strmFlags & 0x20) != 0;
				uint8 strmLocalColorTableSize = (strmFlags & 0x7);

			ZDCPixmapNS::PixelDesc thePixelDesc = fPixelDesc;
			if (strmHasLocalColorTable)
				{
				vector<ZRGBColorPOD> theColors;
				sReadColorTable(iStream, 1 << (strmLocalColorTableSize + 1), theColors);
				thePixelDesc = ZDCPixmapNS::PixelDesc(&theColors[0], theColors.size());
				}

			sReadImageData(iStream, strmIsInterlaced, curBounds, fRaster);
			oPixmap = new ZDCPixmapRep(fRaster, ZRect(fSize), thePixelDesc);
			return;
			}
		}
	}

// =================================================================================================

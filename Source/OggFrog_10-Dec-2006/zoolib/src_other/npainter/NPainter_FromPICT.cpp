// Adapted from picttoppm.c by Tanya Quinn. Reworked by AG to be thread safe and
// avoid the use of a full sized intermediate buffer.

// Note that all of the PICTs saved in Knowledge Forum 2.1.x databases use only the PackBitsRect
// opcode,  so we don't need to deal with all possible PICT opcodes here.
/*
 * picttoppm.c -- convert a MacIntosh PICT file to PPM format.
 *
 * Copyright 1989 George Phillips
 *
 * Permission is granted to freely distribute this program in whole or in
 * part provided you don't make money off it, you don't pretend that you
 * wrote it and that this notice accompanies the code.
 *
 * George Phillips <phillips@cs.ubc.ca>
 * Department of Computer Science
 * University of British Columbia
 */



#include "NPainter_FromPICT.h"
#include "ZDCPixmap.h"
#include "ZStream.h"

static void sReadPixmap(const ZStreamR& inStream, ZDCPixmap& outPixmap);
static void sUnpackFromStream(const ZStreamR& inStream,
	ZCoord inSourceWidth, ZCoord inSourceHeight,
	unsigned short inSourceRowBytes, short inSourcePixelSize,
	const ZRGBColorPOD* inSourceColors, size_t inSourceColorTableSize,
	void* inDestBaseAddress,
	const ZDCPixmapNS::RasterDesc& inDestRasterDesc,
	const ZDCPixmapNS::PixelDesc& inDestPixelDesc);

void NPainter_FromPICT(const ZStreamR& inStream, ZDCPixmap& outPixmap)
	{
	inStream.ReadInt16(); // picSize. This field has been obsolete since 1987.
	inStream.ReadInt16(); // picFrame.top
	inStream.ReadInt16(); // picFrame.left
	inStream.ReadInt16(); // picFrame.bottom
	inStream.ReadInt16(); // picFrame.right

	uint8 ch;

	// skip any empty bytes
	while ((ch = inStream.ReadUInt8()) == 0)
		{}

	if (ch != 0x11)
		throw runtime_error("NPainter_FromPICT, expected version opcode");
	if ((ch = inStream.ReadUInt8()) != 0x02)
		throw runtime_error("NPainter_FromPICT, expected version number 2");
	if ((ch = inStream.ReadUInt8()) != 0xFF)
		throw runtime_error("NPainter_FromPICT, expected subcode 0xFF");

	// The order of remaining opcodes is not always the same, hence we use a loop to consume them.
	uint16 opcode;
	while (true)
		{
		opcode = inStream.ReadUInt16();
		if (opcode == 0x0C00) // Header, followed by 24 bytes of data we don't need
			inStream.Skip(24);
		else if (opcode == 0x01) // Clip rgn
			{
			unsigned short clipBytes = inStream.ReadUInt16() - sizeof(short);
			inStream.Skip(clipBytes);
			}
		else if (opcode != 0x1E) // Hilite
			break;
		}
	if (opcode != 0x98)
		throw runtime_error("NPainter_FromPICT, expected packbits opcode");
	// Do the actual read
	::sReadPixmap(inStream, outPixmap);
	// Suck up remaining data
	while ((ch = inStream.ReadUInt8()) == 0x00)
		{}
	if (ch != 0xFF)
		throw runtime_error("NPainter_FromPICT, expected end of picture opcode");
	}

static void sThrowBadFormat()
	{
	throw runtime_error("NPainter_FromPICT, bad format");
	}

static void sThrowUnsupportedFormat()
	{
	throw runtime_error("NPainter_FromPICT, unsupported format");
	}

static void sReadPixmap(const ZStreamR& inStream, ZDCPixmap& outPixmap)
	{
	uint16 rowBytes = inStream.ReadUInt16();
	if (!(rowBytes & 0x8000))
		::sThrowBadFormat();
	rowBytes &= 0x7FFF;

	ZRect bounds;
	bounds.top = inStream.ReadInt16();
	bounds.left = inStream.ReadInt16();
	bounds.bottom = inStream.ReadInt16();
	bounds.right = inStream.ReadInt16();
	inStream.ReadInt16(); // version
	inStream.ReadInt16(); // packType
	inStream.ReadInt32(); // packSize
	inStream.ReadInt32(); // hRes
	inStream.ReadInt32(); //vRes
	short pixelType = inStream.ReadInt16(); // pixelType
	short pixelSize = inStream.ReadInt16(); // pixelSize
	short cmpCount = inStream.ReadInt16(); // cmpCount
	short cmpSize = inStream.ReadInt16(); // cmpSize
	inStream.ReadInt32(); // planeBytes
	inStream.ReadInt32(); // pmTable
	inStream.ReadInt32(); // pmReserved

	// We only deal with pixel type of 0 (indexed pixels)
	if (pixelType != 0)
		::sThrowUnsupportedFormat();

	// indexed pixels have a cmpCount of 1
	if (cmpCount != 1)
		::sThrowBadFormat();

	// pixelSize and cmpSize should be equal for indexed pixels
	if (pixelSize != cmpSize)
		::sThrowBadFormat();

	// Next on the stream is the color table
	vector<ZRGBColor> theColorTable;
	inStream.ReadInt32(); // ctSeed
	inStream.ReadInt16(); // ctFlags
	short ctSize = inStream.ReadInt16();
	for (int i = 0; i <= ctSize; ++i)
		{
		inStream.ReadInt16(); // colorSpecIndex
		ZRGBColor theColor;
		theColor.red = inStream.ReadUInt16();
		theColor.green = inStream.ReadUInt16();
		theColor.blue = inStream.ReadUInt16();
		theColorTable.push_back(theColor);
		}

	// Now we have the source rect
	inStream.Skip(8);

	// and the destination rect
	inStream.Skip(8);

	// Penultimately we have the mode
	inStream.ReadUInt16();

	// The remaining data is the packed pixels. Allocate our ZDCPixmap
	// using the size we read in, but with no initialized data.
	ZDCPixmap thePixmap(bounds.Size(), ZDCPixmapNS::eFormatEfficient_Color_32);
	void* baseAddress = thePixmap.GetBaseAddress();
	ZDCPixmapNS::RasterDesc theRasterDesc = thePixmap.GetRasterDesc();
	ZDCPixmapNS::PixelDesc thePixelDesc = thePixmap.GetPixelDesc();
	::sUnpackFromStream(inStream,
		bounds.Width(), bounds.Height(),
		rowBytes, pixelSize,
		&theColorTable[0], theColorTable.size(),
		baseAddress, theRasterDesc, thePixelDesc);

	outPixmap = thePixmap;
	}

static void sUnpackFromStream(const ZStreamR& inStream,
	ZCoord inSourceWidth, ZCoord inSourceHeight,
	unsigned short inSourceRowBytes, short inSourcePixelSize,
	const ZRGBColorPOD* inSourceColors, size_t inSourceColorTableSize,
	void* inDestBaseAddress,
	const ZDCPixmapNS::RasterDesc& inDestRasterDesc,
	const ZDCPixmapNS::PixelDesc& inDestPixelDesc)
	{
	// We're only supporting indexed pixels right now
	ZAssert(inSourcePixelSize == 1 || inSourcePixelSize == 2
		|| inSourcePixelSize == 4 || inSourcePixelSize == 8);

	ZDCPixmapNS::PixelDesc sourcePixelDesc(inSourceColors, inSourceColorTableSize);
	ZDCPixmapNS::PixvalDesc sourcePixvalDesc(inSourcePixelSize, true);

	if (inSourceRowBytes < 8)
		{
		// ah-ha!  The bits aren't actually packed. This will be easy.
		uint8 lineBuffer[8];
		for (ZCoord theScanLine = 0; theScanLine < inSourceHeight; ++theScanLine)
			{
			inStream.Read(lineBuffer, inSourceRowBytes);
			void* destRowAddress = inDestRasterDesc.CalcRowAddress(inDestBaseAddress, theScanLine);
			ZDCPixmapNS::sBlitRow(lineBuffer, sourcePixvalDesc, sourcePixelDesc, 0,
				destRowAddress, inDestRasterDesc.fPixvalDesc, inDestPixelDesc, 0,
				inSourceWidth);
			}
		}
	else
		{
		// Sometimes we get rows with length > rowBytes.  Allocate some extra for slop.
		// Also note that the data is stored as multiples of rowBytes, which may represent more
		// pixels than inSourceWidth (if inSourceWidth % 4 != 0). So we have to check for
		// destinationH < inSourceWidth before we actually blit to the destination.
		vector<uint8> lineBufferVector(inSourceRowBytes + 80);
		uint8* lineBuffer = &lineBufferVector[0];
		ZDCPixmapNS::PixvalAccessor destAccessor(inDestRasterDesc.fPixvalDesc);
		for (ZCoord theScanLine = 0; theScanLine < inSourceHeight; ++theScanLine)
			{
			void* destRowAddress = inDestRasterDesc.CalcRowAddress(inDestBaseAddress, theScanLine);

			size_t lineLen;
			if (inSourceRowBytes > 250 || inSourcePixelSize > 8)
				lineLen = inStream.ReadUInt16();
			else
				lineLen = inStream.ReadUInt8();
			if (lineLen > inSourceRowBytes + 80)
				::sThrowBadFormat();
			inStream.Read(lineBuffer, lineLen);

			ZCoord destinationH = 0;
			for (size_t j = 0; j < lineLen; /*no increment*/)
				{
				if (lineBuffer[j] & 0x80)
					{
					size_t repeatCount = (lineBuffer[j++] ^ 0xFF) + 2;
					ZRGBColorPOD theRGBColor;
					sourcePixelDesc.AsRGBColor(lineBuffer[j++], theRGBColor);
					uint32 destPixval = inDestPixelDesc.AsPixval(theRGBColor);
					for (size_t k = 0; k < repeatCount; ++k)
						{
						if (destinationH < inSourceWidth)
							destAccessor.SetPixval(destRowAddress, destinationH, destPixval);
						destinationH += 1;
						}
					}
				else
					{
					size_t realLength = lineBuffer[j] + 1;
					if (inSourceWidth > destinationH)
						{
						size_t countToCopy = min(realLength, size_t(inSourceWidth - destinationH));
						ZDCPixmapNS::sBlitRow(lineBuffer,
							sourcePixvalDesc, sourcePixelDesc, j + 1,
							destRowAddress,
							inDestRasterDesc.fPixvalDesc, inDestPixelDesc, destinationH,
							countToCopy);
						destinationH += countToCopy;
						}
					j += realLength + 1;
					}
				}
			}
		}
	}

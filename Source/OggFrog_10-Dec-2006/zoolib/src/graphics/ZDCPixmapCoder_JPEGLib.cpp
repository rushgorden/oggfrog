static const char rcsid[]
= "@(#) $Id: ZDCPixmapCoder_JPEGLib.cpp,v 1.13 2006/04/26 22:31:48 agreen Exp $";

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

#include "ZDCPixmapCoder_JPEGLib.h"

#if ZCONFIG_SPI_Enabled(JPEGLib)

#if ZCONFIG(OS, Win32) && defined(__ZWinHeader__)
	// Fix up some stuff that breaks jpeglib's jmorecfg.h when we've also already pulled in
	// the Windows. That happens when we're compiling for Windows using ZWinHeader with
	// CodeWarrior's precompiled headers. headers, from ZWinHeader.h.

	// jmorecfg.h unconditionally #defines FAR, so remove any existing definition.
	#undef FAR

	// Prevent jmorecfg.h from typedef-ing INT32 (which basetsd.h has already typedefed.)
	#define XMD_H

	// Prevent jmorecfg.h from typedef-ing boolean (which rpcndr.h has already typedefed.)
	#define HAVE_BOOLEAN

	// But our code still needs to see boolean as being an int.
	#define boolean int
#endif // ZCONFIG(OS, Win32)

extern "C" {
#include <jpeglib.h>
} // extern "C"


// =================================================================================================
#pragma mark -
#pragma mark * JPEGErrorMgr

namespace ZANONYMOUS {

class JPEGErrorMgr : public jpeg_error_mgr
	{
public:
	JPEGErrorMgr();

private:
	static void sErrorExit(j_common_ptr cinfo);
	static void sEmitMessage(j_common_ptr cinfo, int msg_level);
	static void sOutputMessage(j_common_ptr cinfo);
	static void sFormatMessage(j_common_ptr cinfo, char * buffer);
	static void sReset(j_common_ptr cinfo);
	};

JPEGErrorMgr::JPEGErrorMgr()
	{
	error_exit = sErrorExit;
	emit_message = sEmitMessage;
	output_message = sOutputMessage;
	format_message = sFormatMessage;
	reset_error_mgr = sReset;
	}

void JPEGErrorMgr::sErrorExit(j_common_ptr cinfo)
	{
	throw runtime_error("JPEGErrorMgr::sErrorExit");
	}

void JPEGErrorMgr::sEmitMessage(j_common_ptr cinfo, int msg_level)
	{}

void JPEGErrorMgr::sOutputMessage(j_common_ptr cinfo)
	{}

void JPEGErrorMgr::sFormatMessage(j_common_ptr cinfo, char * buffer)
	{}

void JPEGErrorMgr::sReset(j_common_ptr cinfo)
	{}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * JPEGWriter

namespace ZANONYMOUS {

class JPEGWriter : public jpeg_destination_mgr
	{
public:
	JPEGWriter(const ZStreamW& iStream);
	~JPEGWriter();

private:
	static void sInititialize(j_compress_ptr iCInfo);
	static boolean sEmptyOutputBuffer(j_compress_ptr iCInfo);
	static void sTerminate(j_compress_ptr iCInfo);

	const ZStreamW& fStream;
	vector<JOCTET> fBuffer;
	};

JPEGWriter::JPEGWriter(const ZStreamW& iStream)
:	fStream(iStream),
	fBuffer(1024)
	{
	init_destination = sInititialize;
	empty_output_buffer = sEmptyOutputBuffer;
	term_destination = sTerminate;
	}

JPEGWriter::~JPEGWriter()
	{}

void JPEGWriter::sInititialize(j_compress_ptr iCInfo)
	{
	JPEGWriter* theWriter = static_cast<JPEGWriter*>(iCInfo->dest);
	theWriter->next_output_byte = &theWriter->fBuffer[0];
	theWriter->free_in_buffer = theWriter->fBuffer.size();
	}

boolean JPEGWriter::sEmptyOutputBuffer(j_compress_ptr iCInfo)
	{
	JPEGWriter* theWriter = static_cast<JPEGWriter*>(iCInfo->dest);

	theWriter->fStream.Write(&theWriter->fBuffer[0], theWriter->fBuffer.size());
	theWriter->next_output_byte = &theWriter->fBuffer[0];
	theWriter->free_in_buffer = theWriter->fBuffer.size();

	return TRUE;
	}

void JPEGWriter::sTerminate(j_compress_ptr iCInfo)
	{
	JPEGWriter* theWriter = static_cast<JPEGWriter*>(iCInfo->dest);
	if (size_t countToWrite = theWriter->next_output_byte - &theWriter->fBuffer[0])
		theWriter->fStream.Write(&theWriter->fBuffer[0], countToWrite);
	}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * JPEGReader

namespace ZANONYMOUS {

class JPEGReader : public jpeg_source_mgr
	{
public:
	JPEGReader(const ZStreamR& iStream);
	~JPEGReader();

private:
	static void sInititialize(j_decompress_ptr iCInfo);
	static boolean sFillInputBuffer(j_decompress_ptr iCInfo);
	static void sSkip(j_decompress_ptr iCInfo, long num_bytes);
	static void sTerminate(j_decompress_ptr iCInfo);

	const ZStreamR& fStream;
	vector<JOCTET> fBuffer;
	};

JPEGReader::JPEGReader(const ZStreamR& iStream)
:	fStream(iStream),
	fBuffer(1024)
	{
	init_source = sInititialize;
	fill_input_buffer = sFillInputBuffer;
	skip_input_data = sSkip;
	resync_to_restart = jpeg_resync_to_restart;
	term_source = sTerminate;
	}

JPEGReader::~JPEGReader()
	{}

void JPEGReader::sInititialize(j_decompress_ptr iCInfo)
	{
	JPEGReader* theReader = static_cast<JPEGReader*>(iCInfo->src);
	theReader->next_input_byte = &theReader->fBuffer[0];
	theReader->bytes_in_buffer = 0;
	}

boolean JPEGReader::sFillInputBuffer(j_decompress_ptr iCInfo)
	{
	JPEGReader* theReader = static_cast<JPEGReader*>(iCInfo->src);

	size_t countAvailable = theReader->fStream.CountReadable();
	if (countAvailable == 0)
		countAvailable = theReader->fBuffer.size();
	else
		countAvailable = min(countAvailable, theReader->fBuffer.size());

	size_t countRead;
	theReader->fStream.Read(&theReader->fBuffer[0], countAvailable, &countRead);
	if (countRead == 0)
		{
		// Insert a fake EOI marker - as per jpeglib recommendation
		theReader->fBuffer[0] = 0xFF;
		theReader->fBuffer[1] = JPEG_EOI;
		theReader->bytes_in_buffer = 2;
		}
	else
		{
		theReader->next_input_byte = &theReader->fBuffer[0];
		theReader->bytes_in_buffer = countRead;
		}

	return TRUE;
	}

void JPEGReader::sSkip(j_decompress_ptr iCInfo, long num_bytes)
	{
	JPEGReader* theReader = static_cast<JPEGReader*>(iCInfo->src);
	if (theReader->bytes_in_buffer >= num_bytes)
		{
		theReader->bytes_in_buffer -= num_bytes;
		theReader->next_input_byte += num_bytes;
		}
	else
		{
		num_bytes -= theReader->bytes_in_buffer;
		theReader->bytes_in_buffer = 0;
		theReader->fStream.Skip(num_bytes);
		}
	}

void JPEGReader::sTerminate(j_decompress_ptr iCInfo)
	{}

} // anonymous namespace


// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapEncoder_JPEGLib

void ZDCPixmapEncoder_JPEGLib::sWritePixmap(const ZStreamW& iStream,
	const ZDCPixmap& iPixmap, int iQuality)
	{
	ZDCPixmapEncoder_JPEGLib theEncoder(iQuality);
	ZDCPixmapEncoder::sWritePixmap(iStream, iPixmap, theEncoder); 
	}

ZDCPixmapEncoder_JPEGLib::ZDCPixmapEncoder_JPEGLib(int iQuality)
	{
	fQuality = iQuality;
	}

ZDCPixmapEncoder_JPEGLib::~ZDCPixmapEncoder_JPEGLib()
	{}

void ZDCPixmapEncoder_JPEGLib::Imp_Write(const ZStreamW& iStream,
	const void* iBaseAddress,
	const ZDCPixmapNS::RasterDesc& iRasterDesc,
	const ZDCPixmapNS::PixelDesc& iPixelDesc,
	const ZRect& iBounds)
	{
	jpeg_compress_struct theJCS;
	JPEGErrorMgr theEM;
	theJCS.err = &theEM;
	::jpeg_create_compress(&theJCS);

	theJCS.image_width = iBounds.Width();
	theJCS.image_height = iBounds.Height();

	JPEGWriter theJW(iStream);
	theJCS.dest = &theJW;

	vector<uint8> rowBufferVector;
	try
		{
		ZDCPixmapNS::PixvalDesc destPixvalDesc;
		ZDCPixmapNS::PixelDesc destPixelDesc;

		ZRef<ZDCPixmapNS::PixelDescRep> thePixelDescRep = iPixelDesc.GetRep();

		if (ZDCPixmapNS::PixelDescRep_Gray* thePixelDescRep_Gray
			= ZRefDynamicCast<ZDCPixmapNS::PixelDescRep_Gray>(thePixelDescRep))
			{
			theJCS.input_components = 1;
			theJCS.in_color_space = JCS_GRAYSCALE;
			rowBufferVector.resize(iBounds.Width());

			destPixelDesc = ZDCPixmapNS::PixelDesc(ZDCPixmapNS::eFormatStandard_Gray_8);
			destPixvalDesc.fDepth = 8;
			destPixvalDesc.fBigEndian = true;
			}
		else
			{
			theJCS.input_components = 3;
			theJCS.in_color_space = JCS_RGB;
			rowBufferVector.resize(3 * iBounds.Width());

			destPixelDesc = ZDCPixmapNS::PixelDesc(ZDCPixmapNS::eFormatStandard_RGB_24);
			destPixvalDesc.fDepth = 24;
			destPixvalDesc.fBigEndian = true;
			}

		::jpeg_set_defaults(&theJCS);
		::jpeg_set_quality(&theJCS, fQuality, TRUE);
		theJCS.dct_method = JDCT_FASTEST;

		::jpeg_start_compress(&theJCS, TRUE);

		JSAMPROW rowPtr[1];
		rowPtr[0] = &rowBufferVector[0];
		for (size_t y = iBounds.top; y < iBounds.bottom; ++y)
			{
			const void* sourceRowAddress = iRasterDesc.CalcRowAddress(iBaseAddress, y);

			ZDCPixmapNS::sBlitRow(
				sourceRowAddress, iRasterDesc.fPixvalDesc, iPixelDesc, iBounds.left,
				rowPtr[0], destPixvalDesc, destPixelDesc, 0,
				iBounds.Width());

			::jpeg_write_scanlines(&theJCS, rowPtr, 1);
			}

		::jpeg_finish_compress(&theJCS);
		}
	catch (...)
		{
		::jpeg_destroy(reinterpret_cast<j_common_ptr>(&theJCS));
		throw;
		}

	::jpeg_destroy(reinterpret_cast<j_common_ptr>(&theJCS));
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapDecoder_JPEGLib

ZDCPixmap ZDCPixmapDecoder_JPEGLib::sReadPixmap(const ZStreamR& iStream)
	{
	ZDCPixmapDecoder_JPEGLib theDecoder;
	return ZDCPixmapDecoder::sReadPixmap(iStream, theDecoder);
	}

ZDCPixmapDecoder_JPEGLib::ZDCPixmapDecoder_JPEGLib()
	{}

ZDCPixmapDecoder_JPEGLib::~ZDCPixmapDecoder_JPEGLib()
	{}

void ZDCPixmapDecoder_JPEGLib::Imp_Read(const ZStreamR& iStream, ZDCPixmap& oPixmap)
	{
	struct jpeg_decompress_struct theJDS;
	JPEGErrorMgr theEM;
	theJDS.err = &theEM;
			  
	::jpeg_create_decompress(&theJDS);

	JPEGReader theJR(iStream);
	theJDS.src = &theJR;
	try
		{
		::jpeg_read_header(&theJDS, TRUE);
		::jpeg_start_decompress(&theJDS);

		ZDCPixmapNS::PixelDesc sourcePixelDesc;
		ZDCPixmapNS::PixvalDesc sourcePixvalDesc;
		vector<uint8> rowBufferVector;
		if (theJDS.out_color_space == JCS_GRAYSCALE)
			{
			sourcePixelDesc = ZDCPixmapNS::PixelDesc(ZDCPixmapNS::eFormatStandard_Gray_8);
			rowBufferVector.resize(theJDS.image_width);

			sourcePixvalDesc.fDepth = 8;
			sourcePixvalDesc.fBigEndian = true;

			oPixmap = ZDCPixmap(ZPoint(theJDS.image_width, theJDS.image_height),
				ZDCPixmapNS::eFormatEfficient_Gray_8);
			}
		else if (theJDS.out_color_space == JCS_RGB)
			{
			sourcePixelDesc = ZDCPixmapNS::PixelDesc(ZDCPixmapNS::eFormatStandard_RGB_24);
			rowBufferVector.resize(3 * theJDS.image_width);

			sourcePixvalDesc.fDepth = 24;
			sourcePixvalDesc.fBigEndian = true;

			oPixmap = ZDCPixmap(ZPoint(theJDS.image_width, theJDS.image_height),
				ZDCPixmapNS::eFormatEfficient_Color_24);
			}
		else
			{
			// TODO. What about other color spaces?
			ZUnimplemented();
			}

		ZDCPixmapNS::PixelDesc destPixelDesc = oPixmap.GetPixelDesc();
		ZDCPixmapNS::RasterDesc destRasterDesc = oPixmap.GetRasterDesc();
		void* destBaseAddress = oPixmap.GetBaseAddress();

		JSAMPROW rowPtr[1];
		rowPtr[0] = &rowBufferVector[0];
		while (theJDS.output_scanline < theJDS.output_height)
			{
			int scanlinesRead = ::jpeg_read_scanlines(&theJDS, rowPtr, 1);
			ZAssertStop(1, scanlinesRead == 1);

			void* destRowAddress
				= destRasterDesc.CalcRowAddress(destBaseAddress, theJDS.output_scanline - 1);

			ZDCPixmapNS::sBlitRow(
				rowPtr[0], sourcePixvalDesc, sourcePixelDesc, 0,
				destRowAddress, destRasterDesc.fPixvalDesc, destPixelDesc, 0,
				theJDS.image_width);
			}
		::jpeg_finish_decompress(&theJDS);
		}
	catch (...)
		{
		::jpeg_destroy_decompress(&theJDS);
		throw;
		}

	::jpeg_destroy_decompress(&theJDS);
	}

#endif // ZCONFIG_SPI_Enabled(JPEGLib)

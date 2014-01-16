static const char rcsid[]
= "@(#) $Id: ZDCGlyphServer_Asset.cpp,v 1.7 2006/07/23 22:02:51 agreen Exp $";

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

#include "ZDCGlyphServer_Asset.h"
#include "ZString.h"

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

static int16 sReadAsset_Int16(const ZAsset& iAsset)
	{
	try
		{
		if (ZRef<ZStreamerR> theStreamer = iAsset.GetChild("!binary").OpenR())
			return theStreamer->GetStreamR().ReadInt16();
		}
	catch (...)
		{}

	return 0;
	}

static int16 sReadAsset_Int8(const ZAsset& iAsset)
	{
	try
		{
		if (ZRef<ZStreamerR> theStreamer = iAsset.GetChild("!binary").OpenR())
			return theStreamer->GetStreamR().ReadInt8();
		}
	catch (...)
		{}

	return 0;
	}

static string sReadAsset_String(const ZAsset& iAsset)
	{
	try
		{
		if (ZAsset innerAsset = iAsset.GetChild("!string"))
			{
			const void* data;
			size_t size;
			innerAsset.GetData(&data, &size);
			if (data && size)
				return string(static_cast<const char*>(data), size - 1);
			}
		}
	catch (...)
		{}

	return string();
	}

static ZPoint sReadAsset_ZPoint(const ZAsset& iAsset)
	{
	try
		{
		if (ZRef<ZStreamerR> theStreamer = iAsset.GetChild("!binary").OpenR())
			{
			ZPoint result;
			result.h = theStreamer->GetStreamR().ReadInt16();
			result.v = theStreamer->GetStreamR().ReadInt16();
			return result;
			}
		}
	catch (...)
		{}

	return ZPoint(0, 0);
	}

// =================================================================================================
#pragma mark -
#pragma mark * PixmapRaster_Asset

namespace ZANONYMOUS {

class PixmapRaster_Asset : public ZDCPixmapRaster
	{
public:
	PixmapRaster_Asset(const ZAsset& iAsset,
		const void* iBaseAddress, const ZDCPixmapNS::RasterDesc& iRasterDesc);
	virtual ~PixmapRaster_Asset();

private:
	ZAsset fAsset;
	};

PixmapRaster_Asset::PixmapRaster_Asset(const ZAsset& iAsset,
	const void* iBaseAddress, const ZDCPixmapNS::RasterDesc& iRasterDesc)
:	ZDCPixmapRaster(iRasterDesc),
	fAsset(iAsset)
	{
	fBaseAddress = const_cast<void*>(iBaseAddress);
	}

PixmapRaster_Asset::~PixmapRaster_Asset()
	{
	fBaseAddress = nil;
	}

} // anonymous namespace

// =================================================================================================
#pragma mark -
#pragma mark * ZDCGlyphServer_Asset

ZDCGlyphServer_Asset::ZDCGlyphServer_Asset(const ZAsset& iAsset)
:	fAsset(iAsset)
	{}

ZDCGlyphServer_Asset::~ZDCGlyphServer_Asset()
	{}

static ZAsset sGetFontAsset(const ZAsset& iParent, const string& iName)
	{
	if (iName.size())
		{
		if (ZAsset theAsset = iParent.GetChild(iName))
			return theAsset;
		}

	if (ZAssetIter theIter = iParent)
		{
		// Return the first child
		return theIter.Current();
		}
	return ZAsset();
	}

bool ZDCGlyphServer_Asset::GetGlyph(const ZDCFont& iFont, UTF32 iCP,
	ZPoint& oOrigin, ZCoord& oEscapement, ZDCPixmap& oPixmap)
	{
	if (ZAsset fontAsset = sGetFontAsset(fAsset, iFont.GetName()))
		{
		if (ZAsset metricsAsset
			= fontAsset.GetChild("metrics").GetChild(ZString::sFormat("%06X", iCP)))
			{
			oOrigin = sReadAsset_ZPoint(metricsAsset.GetChild("origin"));
			oEscapement = sReadAsset_Int16(metricsAsset.GetChild("escapement"));
			string strikeName = sReadAsset_String(metricsAsset.GetChild("strikename"));
			if (ZAsset strikeAsset = fontAsset.GetChild("strikes").GetChild(strikeName))
				{
				ZCoord offsetInStrike = sReadAsset_Int16(metricsAsset.GetChild("strikeoffset"));
				ZCoord width = sReadAsset_Int16(metricsAsset.GetChild("width"));

				ZDCPixmapNS::RasterDesc theRasterDesc;
				theRasterDesc.fPixvalDesc.fDepth = sReadAsset_Int8(strikeAsset.GetChild("depth"));
				theRasterDesc.fPixvalDesc.fBigEndian = true;
				theRasterDesc.fRowBytes = sReadAsset_Int16(strikeAsset.GetChild("rowBytes"));
				theRasterDesc.fRowCount = sReadAsset_Int16(strikeAsset.GetChild("height"));
				theRasterDesc.fFlipped = false;
				if (ZAsset strikeDataAsset = strikeAsset.GetChild("data|!binary"))
					{
					const void* assetData;
					strikeDataAsset.GetData(&assetData, nil);

					ZRef<ZDCPixmapRaster> theRaster
						= new PixmapRaster_Asset(strikeDataAsset, assetData, theRasterDesc);
					
					ZRect theBounds(width, theRasterDesc.fRowCount);
					theBounds += ZPoint(offsetInStrike, 0);

					ZDCPixmapNS::PixelDesc thePixelDesc(uint32(0),
						(1 << theRasterDesc.fPixvalDesc.fDepth) - 1);

					oPixmap = ZDCPixmapFactory::sCreateRep(theRaster, theBounds, thePixelDesc);
					return true;
					}
				}
			}
		}
	return false;
	}

ZCoord ZDCGlyphServer_Asset::GetEscapement(const ZDCFont& iFont, UTF32 iCP)
	{
	if (ZAsset fontAsset = sGetFontAsset(fAsset, iFont.GetName()))
		{
		if (ZAsset metricsAsset
			= fontAsset.GetChild("metrics").GetChild(ZString::sFormat("%06X", iCP)))
			{
			return sReadAsset_Int16(metricsAsset.GetChild("escapement"));
			}
		}
	return 0;
	}

void ZDCGlyphServer_Asset::GetFontInfo(const ZDCFont& iFont,
	ZCoord& oAscent, ZCoord& oDescent, ZCoord& oLeading)
	{
	oAscent = 1;
	oDescent = 1;
	oLeading = 0;
	if (ZAsset fontAsset = sGetFontAsset(fAsset, iFont.GetName()))
		{
		oAscent = sReadAsset_Int16(fontAsset.GetChild("ascent"));
		oDescent = sReadAsset_Int16(fontAsset.GetChild("descent"));
		oLeading = sReadAsset_Int16(fontAsset.GetChild("leading"));
		}	
	}

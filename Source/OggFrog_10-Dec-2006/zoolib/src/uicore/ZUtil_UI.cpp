static const char rcsid[] = "@(#) $Id: ZUtil_UI.cpp,v 1.15 2006/04/10 20:44:22 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2000 Andrew Green and Learning in Motion, Inc.
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

#include "ZDebug.h"
#include "ZDCPixmap_Asset_BMP.h"
#include "ZFile.h"
#include "ZStream_Limited.h"
#include "ZStream_String.h"
#include "ZString.h"
#include "ZTextCoder_Unicode.h"
#include "ZUIRendered.h"
#include "ZUIRenderer_Platinum.h"
#include "ZUIRenderer_Appearance.h"
#include "ZUIRenderer_Win32.h"
#include "ZUtil_UI.h"
#include "ZUtil_Mac_HL.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <Resources.h>
#endif

#if ZCONFIG(API_Graphics, GDI)
#	include "ZWinHeader.h"
#endif

#if ZCONFIG(OS, Be)
#	include <app/Application.h>
#	include <storage/Resources.h>
#endif

// =================================================================================================

void ZUtil_UI::sInitializeUIFactories()
	{
	ZRef<ZUIFactory> theUIFactory;
	ZRef<ZUIAttributeFactory> theUIAttributeFactory;

	#if ZCONFIG(OS, Win32)
		if (!theUIFactory && !theUIAttributeFactory)
			{
			theUIFactory = new ZUIFactory_Rendered(new ZUIRendererFactory_Win32);
			theUIAttributeFactory = new ZUIAttributeFactory_Win32;
			}
	#endif


	#if ZCONFIG_API_Enabled(Appearance)
		if (!theUIFactory && !theUIAttributeFactory && ZUIRenderer_Appearance::sCanUse())
			{
			theUIFactory = new ZUIFactory_Rendered(new ZUIRendererFactory_Appearance);
			theUIAttributeFactory = new ZUIAttributeFactory_Appearance;
			}
	#endif


	#if ZCONFIG_UIRenderer_PlatinumAvailable
		if (!theUIFactory && !theUIAttributeFactory)
			{
			theUIFactory = new ZUIFactory_Rendered(new ZUIRendererFactory_Platinum);
			theUIAttributeFactory = new ZUIAttributeFactory_Platinum;
			}
	#endif

	ZUIFactory::sSet(theUIFactory);
	ZUIAttributeFactory::sSet(theUIAttributeFactory);
	}

void ZUtil_UI::sTearDownUIFactories()
	{
	ZUIFactory::sSet(ZRef<ZUIFactory>());
	ZUIAttributeFactory::sSet(ZRef<ZUIAttributeFactory>());
	}

// =================================================================================================

ZDCPixmapCombo ZUtil_UI::sGetPixmapCombo(const ZAsset& inAsset)
	{
	ZDCPixmap thePixmapColor = sGetPixmap(inAsset.GetChild("Color"));
	ZDCPixmap thePixmapMono = sGetPixmap(inAsset.GetChild("Mono"));
	ZDCPixmap thePixmapMask = sGetPixmap(inAsset.GetChild("Mask"));
	return ZDCPixmapCombo(thePixmapColor, thePixmapMono, thePixmapMask);
	}

ZDCPixmap ZUtil_UI::sGetPixmap(const ZAsset& inAsset)
	{
#if 1
	// This code returns a pixmap that tries to use the data stored in the asset
	// as the storage for its raster. If the data is RLE compressed, and thus
	// not describable by ZDCPixmapNS::RasterDesc then it falls back to
	// interpreting the RLE opcodes and allocates storage in the normal way.
	return ZDCPixmap_Asset_BMP::sGetPixmap(inAsset.GetChild("!binary"));

#else

	if (ZRef<ZStreamerR> theStreamer = inAsset.GetChild("!binary").OpenR())
		return ZDCPixmapDecoder_BMP::sReadPixmap(theStreamer->GetStreamR(), true);

#endif

	return ZDCPixmap();
	}

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

ZCursor ZUtil_UI::sGetCursor(const ZAsset& inAsset)
	{
	ZAsset hotSpotAsset = inAsset.GetChild("HotSpot");
	ZAsset colorAsset = inAsset.GetChild("Color");
	ZAsset monoAsset = inAsset.GetChild("Mono");
	ZAsset maskAsset = inAsset.GetChild("Mask");
	if (hotSpotAsset && ((colorAsset && maskAsset) || monoAsset))
		{
		ZPoint hotSpot(0, 0);
		if (ZRef<ZStreamerR> theStreamer = hotSpotAsset.GetChild("!binary").OpenR())
			{
			hotSpot.h = theStreamer->GetStreamR().ReadInt16();
			hotSpot.v = theStreamer->GetStreamR().ReadInt16();
			}
		ZDCPixmap colorPixmap = sGetPixmap(colorAsset);
		ZDCPixmap monoPixmap = sGetPixmap(monoAsset);
		ZDCPixmap maskPixmap = sGetPixmap(maskAsset);
		if (!maskPixmap && monoPixmap)
			{
			// Make a mask by copying and inverting the mono pixmap
			maskPixmap = monoPixmap;
			maskPixmap.Munge(sMungeProc_Invert, nil);			
			}
		return ZCursor(colorPixmap, monoPixmap, maskPixmap, hotSpot);
		}
	return ZCursor();
	}

string ZUtil_UI::sGetString(const ZAsset& inAsset)
	{
	string result;
	if (ZAsset innerAsset = inAsset.GetChild("!string"))
		{
		const void* data;
		size_t size;
		innerAsset.GetData(&data, &size);
		if (data && size)
			return string(static_cast<const char*>(data), size - 1);
		}
	return result;
	}

string ZUtil_UI::sGetString(const ZAsset& inAsset, const char* inChildName)
	{ return sGetString(inAsset.GetChild(inChildName)); }

static uint32 sReadCount(const ZStreamR& inStream)
	{
	uint8 firstByte = inStream.ReadUInt8();
	if (firstByte < 0xFF)
		return firstByte;
	return inStream.ReadUInt32();
	}

string ZUtil_UI::sGetString(const ZAsset& inAsset, size_t inIndex)
	{
	string result;
	if (ZRef<ZStreamerR> theStreamer = inAsset.GetChild("!stringtable").OpenR())
		{
		const ZStreamR& theStream = theStreamer->GetStreamR();
		size_t theCount = sReadCount(theStream);
		for (size_t currentIndex = 0; currentIndex < theCount; ++currentIndex)
			{
			size_t stringSize = sReadCount(theStream);
			if (currentIndex == inIndex)
				{
				ZStreamWPos_String(result).CopyFrom(theStream, stringSize, nil, nil);
				break;
				}
			theStream.Skip(stringSize + 1);
			}
		}
	return result;
	}

// =================================================================================================

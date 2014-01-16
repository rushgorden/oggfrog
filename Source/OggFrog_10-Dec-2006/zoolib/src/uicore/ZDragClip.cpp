static const char rcsid[] = "@(#) $Id: ZDragClip.cpp,v 1.14 2006/07/15 20:54:22 goingware Exp $";

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

#include "ZDragClip.h"
#include "ZDragClip_Win.h"
#include "ZMemory.h" // for ZBlockMove
#include "ZOSWindow_Win.h" // For ZOSApp_Win::InvokeFunctionFromMessagePump
#include "ZUnicode.h"
#include "ZUtil_Win.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
#	include "ZHandle.h"
#endif

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <Scrap.h>
#endif

#if ZCONFIG(OS, MacOSX)
#	include <HIToolbox/Scrap.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Windows helper functions

#if ZCONFIG(OS, Win32)

/** \fixme The OLE stuff got moved out of ZOSApp_Win in order that we could
run in browser plugins. That's still something we'll be wanting to do, so
the clipboard stuff may need to be re-revised (again) to use different
strategies in different runtime environments. */

static void* sWin32_Get_Callback(void* iParam)
	{
	ZTuple* result = static_cast<ZTuple*>(iParam);

	IDataObject* theIDataObject;
	HRESULT theHRESULT = ::OleGetClipboard(&theIDataObject);
	if (SUCCEEDED(theHRESULT))
		{
		*result = ZDragClip_Win_DataObject::sAsTuple(theIDataObject);
		theIDataObject->Release();
		}
	else
		{
		result->Clear();
		}
	return nil;
	}

static ZTuple sWin32_Get()
	{
	ZTuple result;
	ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sWin32_Get_Callback, &result);
	return result;
	}

static void* sWin32_Set_Callback(void* iParam)
	{
	const ZTuple& theTuple = *static_cast<ZTuple*>(iParam);
	if (theTuple.Empty())
		{
		::OleFlushClipboard();
		}
	else
		{
		ZDragClip_Win_DataObject* theDataObject = new ZDragClip_Win_DataObject(theTuple);
		theDataObject->AddRef();
		IDataObject* theIDataObject;
		if (SUCCEEDED(theDataObject->QueryInterface(IID_IDataObject, (void**)&theIDataObject)) && theIDataObject)
			{
			HRESULT theHRESULT = ::OleSetClipboard(theIDataObject);
			theIDataObject->Release();
			}
		theDataObject->Release();
		}
	return nil;
	}

static void sWin32_Set(const ZTuple& iTuple)
	{
	ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sWin32_Set_Callback, const_cast<ZTuple*>(&iTuple));
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZClipboard

ZClipboard* ZClipboard::sClipboard;

ZClipboard* ZClipboard::sGetClipboard()
	{
	if (!sClipboard)
		new ZClipboard;
	return sClipboard;
	}

ZClipboard::ZClipboard()
	{
	ZAssert(!sClipboard);
	sClipboard = this;

#if ZCONFIG(OS, MacOS7)
	fLastKnownScrapCount = ::InfoScrap()->scrapCount - 1;
#endif // ZCONFIG(OS, MacOS7)
	}

ZClipboard::~ZClipboard()
	{
	ZAssert(sClipboard == this);
	sClipboard = nil;
	}

ZTuple ZClipboard::sGet()
	{ return sGetClipboard()->Get(); }

void ZClipboard::sSet(const ZTuple& inTuple)
	{ sGetClipboard()->Set(inTuple); }

void ZClipboard::sClear()
	{ sGetClipboard()->Clear(); }

void ZClipboard::Set(const ZTuple& iTuple)
	{
#if ZCONFIG(OS, MacOS7)

	fTuple = iTuple;
	::ZeroScrap();
	// Push registered types out to the desk scrap
	for (ZTuple::const_iterator i = fTuple.begin(); i != fTuple.end(); ++i)
		{
		string propertyName = fTuple.NameOf(i);
		ScrapFlavorType theScrapFlavorType;
		bool isString;
		if (ZDragClipManager::sGet()->LookupMIME(propertyName, theScrapFlavorType, isString))
			{
			ZType propertyType = fTuple.TypeOf(i);
			if (isString && propertyType == eZType_String)
				{
				string theString = fTuple.GetString(propertyName);
				if (theString.size())
					::PutScrap(theString.size(), theScrapFlavorType, theString.data());
				else
					::PutScrap(0, theScrapFlavorType, nil);
				}
			else if (!isString && propertyType == eZType_Raw)
				{
				size_t rawSize;
				const void* rawAddress;
				bool okay = fTuple.GetRawAttributes(propertyName, &rawAddress, &rawSize);
				ZAssertStop(1, okay);
				::PutScrap(rawSize, theScrapFlavorType, rawAddress);
				}
			}
		}
	fLastKnownScrapCount = ::InfoScrap()->scrapCount;

#elif ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	fTuple = iTuple;
	::ClearCurrentScrap();
	ScrapRef currentScrapRef;
	::GetCurrentScrap(&currentScrapRef);
	// Push registered types out to the desk scrap
	for (ZTuple::const_iterator i = fTuple.begin(); i != fTuple.end(); ++i)
		{
		string propertyName = fTuple.NameOf(i);
		ScrapFlavorType flavorType;
		bool isString;
		if (ZDragClipManager::sGet()->LookupMIME(propertyName, flavorType, isString))
			{
			ZType propertyType = fTuple.TypeOf(i);
			if (isString && propertyType == eZType_String)
				{
				string theString = fTuple.GetString(propertyName);
				if (theString.size())
					::PutScrapFlavor(currentScrapRef, flavorType, kScrapFlavorMaskNone, theString.size(), theString.data());
				else
					::PutScrapFlavor(currentScrapRef, flavorType, kScrapFlavorMaskNone, 0, nil);
				}
			else if (!isString && propertyType == eZType_Raw)
				{
				size_t rawSize;
				const void* rawAddress;
				bool okay = fTuple.GetRawAttributes(propertyName, &rawAddress, &rawSize);
				ZAssertStop(1, okay);
				::PutScrapFlavor(currentScrapRef, flavorType, kScrapFlavorMaskNone, rawSize, rawAddress);
				}
			}
		}
	::GetCurrentScrap(&fLastKnownScrapRef);

#elif ZCONFIG(OS, Win32)

	sWin32_Set(iTuple);

#else

	ZDebugLogf(0, ("ZClipboard::Put(), Unsupported platform"));

#endif
	}

ZTuple ZClipboard::Get()
	{
#if ZCONFIG(OS, MacOS7)

	SInt16 currentScrapCount = ::InfoScrap()->scrapCount;
	if (fLastKnownScrapCount != currentScrapCount)
		{
		fLastKnownScrapCount = currentScrapCount;

		fTuple = ZTuple();
		// Walk through all the registered types and suck any data that might be on the clipboard into our tuple

		size_t theCount = ZDragClipManager::sGet()->Count();
		for (size_t x = 0; x < theCount; ++x)
			{
			string currentMIME = ZDragClipManager::sGet()->At(x);
			bool isString;
			ScrapFlavorType theScrapFlavorType;
			if (ZDragClipManager::sGet()->LookupMIME(currentMIME, theScrapFlavorType, isString))
				{
				SInt32 theOffset = 0;
				SInt32 theLength = ::GetScrap(nil, theScrapFlavorType, &theOffset);
				if (theLength >= 0)
					{
					// Creates a zero-length handle, rather than a NULL handle
					ZHandle theHandle(0);
					theLength = ::GetScrap(theHandle.GetMacHandle(), theScrapFlavorType, &theOffset);
					// AG 99-01-07. For the moment we're ignoring the value of theOffset, which indicates the order in which
					// different data items were written to the desk scrap, and hence their preference level.
					if (theHandle.GetSize() == theLength)
						{
						ZHandle::Ref theRef(theHandle);
						if (isString)
							fTuple.SetString(currentMIME, string(reinterpret_cast<char*>(theRef.GetData()), theRef.GetSize()));
						else
							fTuple.SetRaw(currentMIME, theRef.GetData(), theRef.GetSize());
						}
					}
				}
			}
		}
	return fTuple;

#elif ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	ScrapRef currentScrapRef;
	::GetCurrentScrap(&currentScrapRef);
	if (fLastKnownScrapRef != currentScrapRef)
		{
		fLastKnownScrapRef = currentScrapRef;

		fTuple = ZTuple();
		// Walk through all the registered types and suck any data that might be on the clipboard into our tuple
		size_t theCount = ZDragClipManager::sGet()->Count();
		for (size_t x = 0; x < theCount; ++x)
			{
			string currentMIME = ZDragClipManager::sGet()->At(x);
			bool isString;
			ScrapFlavorType flavorType;
			if (ZDragClipManager::sGet()->LookupMIME(currentMIME, flavorType, isString))
				{
				ScrapFlavorFlags flavorFlags;
				if (noErr == ::GetScrapFlavorFlags(currentScrapRef, flavorType, &flavorFlags))
					{
					Size availByteCount = 0;
					OSStatus status = ::GetScrapFlavorSize(currentScrapRef, flavorType, &availByteCount);
					if ((status == noErr) && (availByteCount >= 0))
						{
						vector<uint8> buffer(availByteCount);
						Size actualByteCount = availByteCount;
						status = ::GetScrapFlavorData(currentScrapRef, flavorType, &actualByteCount, &buffer[0]);
						if (actualByteCount == availByteCount)
							{
							if (isString)
								fTuple.SetString(currentMIME, string(reinterpret_cast<char*>(&buffer[0]), availByteCount));
							else
								fTuple.SetRaw(currentMIME, &buffer[0], availByteCount);
							}
						}
					}
				}
			}
		}
	return fTuple;

#elif ZCONFIG(OS, Win32)

	return sWin32_Get();

#else

	ZDebugLogf(0, ("ZClipboard::Get(), Unsupported platform"));
	return ZTuple();

#endif
	}

void ZClipboard::Clear()
	{
#if ZCONFIG(OS, MacOS7)

	if (!fTuple.Empty())
		{
		fTuple = ZTuple();
		::ZeroScrap();
		fLastKnownScrapCount = ::InfoScrap()->scrapCount;
		}

#elif ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	if (!fTuple.Empty())
		{
		fTuple = ZTuple();
		::ClearScrap(&fLastKnownScrapRef);
		}

#elif ZCONFIG(OS, Win32)

	sWin32_Set(ZTuple());

#else

	ZDebugLogf(0, ("ZClipboard::Empty(), Unsupported platform"));

#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClipManager

ZDragClipManager* ZDragClipManager::sDragClipManager = nil;

ZDragClipManager::ZDragClipManager()
	{
	ZAssertStop(1, sDragClipManager == nil);
	sDragClipManager = this;

// Get the initial MIME type mappings installed

	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
		this->RegisterMIME("text/plain", 'TEXT', true);
		this->RegisterMIME("image/x-PICT", 'PICT', false);
	#endif

	#if ZCONFIG(OS, Win32)
		this->RegisterMIME("text/plain", CF_TEXT, true);
		this->RegisterMIME("image/x-BMP", CF_DIB, false);
	#endif
	}

ZDragClipManager::~ZDragClipManager()
	{
	ZAssertStop(1, sDragClipManager == this);
	sDragClipManager = nil;
	}

ZDragClipManager* ZDragClipManager::sGet()
	{
	if (sDragClipManager == nil)
		new ZDragClipManager;
	return sDragClipManager;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClipManager MacOS-specific

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

size_t ZDragClipManager::Count()
	{ return fMIMENativeVector.size(); }

string ZDragClipManager::At(size_t iIndex)
	{ return fMIMENativeVector[iIndex].fMIME; }

void ZDragClipManager::RegisterMIME(const string& iMIME, ScrapFlavorType iScrapFlavorType, bool iIsString)
	{
	ZAssertStop(1, !iMIME.empty());

	if (ZCONFIG_Debug)
		{
		for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
			{
			if (i->fMIME == iMIME)
				{
				ZDebugStopf(1, ("Attempting to register a MIME we already have"));
				return;
				}
			}
		}

	MIMENative theMIMENative;
	theMIMENative.fMIME = iMIME;
	theMIMENative.fScrapFlavorType = iScrapFlavorType;
	theMIMENative.fIsString = iIsString;
	fMIMENativeVector.push_back(theMIMENative);
	}

bool ZDragClipManager::LookupScrapFlavorType(ScrapFlavorType iScrapFlavorType, string& oMIME, bool& oIsString)
	{
	for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
		{
		if (i->fScrapFlavorType == iScrapFlavorType)
			{
			oMIME = i->fMIME;
			oIsString = i->fIsString;
			return true;
			}
		}
	return false;
	}

bool ZDragClipManager::LookupMIME(const string& iMIME, ScrapFlavorType& oScrapFlavorType, bool& oIsString)
	{
	for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
		{
		if (i->fMIME == iMIME)
			{
			oScrapFlavorType = i->fScrapFlavorType;
			oIsString = i->fIsString;
			return true;
			}
		}
	return false;
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClipManager Win32-specific

#if ZCONFIG(OS, Win32)

void ZDragClipManager::RegisterMIME(const string& iMIME, bool iIsString)
	{
	ZAssertStop(1, !iMIME.empty());

	if (ZCONFIG_Debug)
		{
		for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
			{
			if (i->fMIME == iMIME)
				{
				ZDebugStopf(1, ("Attempting to register a MIME we already have"));
				return;
				}
			}
		}

	CLIPFORMAT newClipboardFormat;
	if (ZUtil_Win::sUseWAPI())
		newClipboardFormat = ::RegisterClipboardFormatW(ZUnicode::sAsUTF16(iMIME).c_str());
	else
		newClipboardFormat = ::RegisterClipboardFormatA(iMIME.c_str());

	MIMENative theMIMENative;
	theMIMENative.fMIME = iMIME;
	theMIMENative.fCLIPFORMAT = newClipboardFormat;
	theMIMENative.fIsString = iIsString;

	fMIMENativeVector.push_back(theMIMENative);
	}

void ZDragClipManager::RegisterMIME(const string& iMIME, CLIPFORMAT iCLIPFORMAT, bool iIsString)
	{
	ZAssertStop(1, !iMIME.empty());

	if (ZCONFIG_Debug)
		{
		for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
			{
			if (i->fMIME == iMIME)
				{
				ZDebugStopf(1, ("Attempting to register a MIME we already have"));
				return;
				}
			}
		}

	MIMENative theMIMENative;
	theMIMENative.fMIME = iMIME;
	theMIMENative.fCLIPFORMAT = iCLIPFORMAT;
	theMIMENative.fIsString = iIsString;

	fMIMENativeVector.push_back(theMIMENative);
	}

bool ZDragClipManager::LookupCLIPFORMAT(CLIPFORMAT iCLIPFORMAT, string& oMIME, bool& oIsString)
	{
	for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
		{
		if (i->fCLIPFORMAT == iCLIPFORMAT)
			{
			oMIME = i->fMIME;
			oIsString = i->fIsString;
			return true;
			}
		}
	return false;
	}

bool ZDragClipManager::LookupMIME(const string& iMIME, CLIPFORMAT& oCLIPFORMAT, bool& oIsString)
	{
	for (vector<MIMENative>::iterator i = fMIMENativeVector.begin(); i != fMIMENativeVector.end(); ++i)
		{
		if (i->fMIME == iMIME)
			{
			oCLIPFORMAT = i->fCLIPFORMAT;
			oIsString = i->fIsString;
			return true;
			}
		}
	return false;
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZDragSource

ZDragSource::ZDragSource()
	{}

ZDragSource::~ZDragSource()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragDropHandler

ZDragDropHandler::ZDragDropHandler()
	{}

ZDragDropHandler::~ZDragDropHandler()
	{}

void ZDragDropHandler::HandleDrag(ZSubPane* iSubPane, ZPoint iPoint, ZMouse::Motion iMotion, ZDrag& iDrag)
	{}

void ZDragDropHandler::HandleDrop(ZSubPane* iSubPane, ZPoint iPoint, ZDrop& iDrop)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragInitiator

ZDragInitiator::ZDragInitiator()
	{}

ZDragInitiator::~ZDragInitiator()
	{}

ZDCRgn ZDragInitiator::OutlineRgn(const ZDCRgn& iRgn)
	{
	// Provides a place to do theme-sensitive outlining of regions (2 pixels on MacOS 8.5, 1 pixel just about everywhere else)
	return iRgn - iRgn.Inset(2, 2);
	}

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, nil, nil, nil, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, nil, nil, nil, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, nil, &iDragMaskPixmap, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, nil, &iDragMaskPixmap, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, &iDragMaskRgn, nil, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, &iDragMaskRgn, nil, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, nil, nil, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCPixmap& iDragPixmap, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, nil, &iDragPixmap, nil, nil, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, nil, &iDragMaskPixmap, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCPixmap& iDragMaskPixmap, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, nil, &iDragMaskPixmap, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, &iDragMaskRgn, nil, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, const ZDCRgn& iDragMaskRgn, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, &iDragMaskRgn, nil, iDragSource); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, nil, nil, nil); }

void ZDragInitiator::DoDrag(const ZTuple& iTuple, ZPoint iStartPoint, ZPoint iHotPoint, const ZDCRgn& iDragRgn, const ZDCPixmap& iDragPixmap, ZDragSource* iDragSource)
	{ this->Imp_DoDrag(iTuple, iStartPoint, iHotPoint, &iDragRgn, &iDragPixmap, nil, nil, iDragSource); }

// =================================================================================================

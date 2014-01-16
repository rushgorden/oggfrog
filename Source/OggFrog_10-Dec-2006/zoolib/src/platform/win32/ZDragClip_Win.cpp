static const char rcsid[] = "@(#) $Id: ZDragClip_Win.cpp,v 1.10 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZDragClip_Win.h"

#if ZCONFIG(OS, Win32)

#include "ZMemory.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_DataObject

const IID ZDragClip_Win_DataObject::sIID = // ("54988d67-a074-11d2-8f58-00c0f023db44")
	{
	0x54988d67,
	0xa074,
	0x11d2,
	{ 0x8f, 0x58, 0x00, 0xc0, 0xf0, 0x23, 0xdb, 0x44}
	};

ZDragClip_Win_DataObject::ZDragClip_Win_DataObject(const ZTuple& iTuple)
:	fTuple(iTuple)
	{
	ZThreadSafe_Set(fRefCount, 0);
	}

ZDragClip_Win_DataObject::~ZDragClip_Win_DataObject()
	{
	ZAssertStop(1, ZThreadSafe_Get(fRefCount) == 0);
	}

STDMETHODIMP ZDragClip_Win_DataObject::QueryInterface(const IID& iInterfaceID, void** oObjectRef)
	{
	*oObjectRef = nil;
	if (iInterfaceID == IID_IUnknown || iInterfaceID == IID_IDataObject)
		{
		*oObjectRef = static_cast<IDataObject*>(this);
		this->AddRef();
		return NOERROR;
		}

	if (iInterfaceID == ZDragClip_Win_DataObject::sIID)
		{
		*oObjectRef = this;
		this->AddRef();
		return NOERROR;
		}

	return E_NOINTERFACE;
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_DataObject::AddRef()
	{
	return ZThreadSafe_IncReturnNew(fRefCount);
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_DataObject::Release()
	{
	if (int newValue = ZThreadSafe_DecReturnNew(fRefCount))
		return newValue;
	delete this;
	return 0;
	}

// From IDataObject
STDMETHODIMP ZDragClip_Win_DataObject::GetData(FORMATETC* iRequestedFORMATETC, STGMEDIUM* oSTGMEDIUM)
	{
	ZAssertStop(1, iRequestedFORMATETC && oSTGMEDIUM);
	if (iRequestedFORMATETC->tymed & TYMED_HGLOBAL)
		{
		bool isString;
		string theRequestedMIME;
		if (ZDragClipManager::sGet()->LookupCLIPFORMAT(iRequestedFORMATETC->cfFormat, theRequestedMIME, isString))
			{
			ZTuple::const_iterator iter = fTuple.IteratorOf(theRequestedMIME);
			if (iter != fTuple.end())
				{
				ZType propertyType = fTuple.TypeOf(iter);
				if (propertyType == eZType_Raw)
					{
					const void* theAddress;
					size_t theSize;
					fTuple.GetRawAttributes(iter, &theAddress, &theSize);
					HGLOBAL theHGLOBAL = ::GlobalAlloc(0, theSize);
					void* globalPtr = ::GlobalLock(theHGLOBAL);
					ZBlockCopy(theAddress, globalPtr, theSize);
					::GlobalUnlock(theHGLOBAL);
					oSTGMEDIUM->tymed = TYMED_HGLOBAL;
					oSTGMEDIUM->hGlobal = theHGLOBAL;
					oSTGMEDIUM->pUnkForRelease = nil;
					return NOERROR;
					}
				else if (propertyType == eZType_String)// && iRequestedFORMATETC->cfFormat == CF_TEXT)
					{
					string theString = fTuple.GetString(iter);
					size_t theSize = theString.size();
					HGLOBAL theHGLOBAL = ::GlobalAlloc(0, theSize + 1);
					void* globalPtr = ::GlobalLock(theHGLOBAL);
					if (theSize)
						ZBlockCopy(theString.data(), globalPtr, theSize);
					reinterpret_cast<char*>(globalPtr)[theSize] = 0;
					::GlobalUnlock(theHGLOBAL);
					oSTGMEDIUM->tymed = TYMED_HGLOBAL;
					oSTGMEDIUM->hGlobal = theHGLOBAL;
					oSTGMEDIUM->pUnkForRelease = nil;
					return NOERROR;
					}
				}
			}
		}
	return E_FAIL;
	}

STDMETHODIMP ZDragClip_Win_DataObject::GetDataHere(FORMATETC*, STGMEDIUM*)
	{ return E_NOTIMPL; }

STDMETHODIMP ZDragClip_Win_DataObject::QueryGetData(FORMATETC* iRequestedFORMATETC)
	{
	ZAssertStop(1, iRequestedFORMATETC);
	if (iRequestedFORMATETC->tymed & TYMED_HGLOBAL)
		{
		bool isString;
		string theRequestedMIME;
		if (ZDragClipManager::sGet()->LookupCLIPFORMAT(iRequestedFORMATETC->cfFormat, theRequestedMIME, isString))
			{
			if (fTuple.Has(theRequestedMIME))
				return NOERROR;
			}
		}
	return S_FALSE;
	}

STDMETHODIMP ZDragClip_Win_DataObject::GetCanonicalFormatEtc(FORMATETC*, FORMATETC*)
	{ return E_NOTIMPL; }

STDMETHODIMP ZDragClip_Win_DataObject::SetData(FORMATETC*, STGMEDIUM*, BOOL)
	{ return E_NOTIMPL; }

STDMETHODIMP ZDragClip_Win_DataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc)
	{
	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;
	*ppenumFormatEtc = new ZDragClip_Win_Enum(this, 0);
	(*ppenumFormatEtc)->AddRef();
	return NOERROR;
	}

STDMETHODIMP ZDragClip_Win_DataObject::DAdvise(FORMATETC*, DWORD, LPADVISESINK, DWORD *)
	{ return E_NOTIMPL; }

STDMETHODIMP ZDragClip_Win_DataObject::DUnadvise(DWORD)
	{ return E_NOTIMPL; }

STDMETHODIMP ZDragClip_Win_DataObject::EnumDAdvise(LPENUMSTATDATA *)
	{ return E_NOTIMPL; }

const ZTuple* ZDragClip_Win_DataObject::GetTuple() const
	{ return &fTuple; }

static void sFilterNonHGLOBAL(IDataObject* iDataObject, vector<FORMATETC>& oVector)
	{
	IEnumFORMATETC* theEnum;
	if (SUCCEEDED(iDataObject->EnumFormatEtc(DATADIR_GET, &theEnum)))
		{
		FORMATETC theFORMATETC;
		ULONG returnedCount;
		while (SUCCEEDED(theEnum->Next(1, &theFORMATETC, &returnedCount)) && returnedCount != 0)
			{
			if (theFORMATETC.tymed & TYMED_HGLOBAL)
				oVector.push_back(theFORMATETC);
			}
		theEnum->Release();
		}
	}

ZTuple ZDragClip_Win_DataObject::sAsTuple(IDataObject* iIDataObject)
	{
	ZTuple theTuple;
	ZDragClip_Win_DataObject* theDataObject;
	if (SUCCEEDED(iIDataObject->QueryInterface(ZDragClip_Win_DataObject::sIID, (void**)&theDataObject)))
		{
		theTuple = *theDataObject->GetTuple();
		theDataObject->Release();
		}
	else
		{
		vector<FORMATETC> filteredVector;
		::sFilterNonHGLOBAL(iIDataObject, filteredVector);

		for (vector<FORMATETC>::const_iterator i = filteredVector.begin();i != filteredVector.end(); ++i)
			{
			bool isString;
			string thePropertyName;
			if (ZDragClipManager::sGet()->LookupCLIPFORMAT((*i).cfFormat, thePropertyName, isString))
				{
				ZAssertStop(1, !thePropertyName.empty());
				FORMATETC theFORMATETC = *i;
				STGMEDIUM theSTGMEDIUM;
				HRESULT theHRESULT = iIDataObject->GetData(&theFORMATETC, &theSTGMEDIUM);
				if (SUCCEEDED(theHRESULT))
					{
					if (theSTGMEDIUM.tymed & TYMED_HGLOBAL)
						{
						void* globalPtr = ::GlobalLock(theSTGMEDIUM.hGlobal);
						// Special case text, to find and ignore the zero terminator, and to store a string.
						if (theFORMATETC.cfFormat == CF_TEXT)
							theTuple.SetString(thePropertyName, string(reinterpret_cast<char*>(globalPtr)));
						else
							theTuple.SetRaw(thePropertyName, globalPtr, ::GlobalSize(theSTGMEDIUM.hGlobal));

						::GlobalUnlock(theSTGMEDIUM.hGlobal);
						}
					::ReleaseStgMedium(&theSTGMEDIUM);
					}
				}
			}
		}
	return theTuple;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_Enum

ZDragClip_Win_Enum::ZDragClip_Win_Enum(const ZTuple& iTuple, ZTuple::const_iterator iCurrentIter)
:	fTuple(iTuple),
	fCurrentIter(iCurrentIter)
	{
	ZThreadSafe_Set(fRefCount, 0);
	}

ZDragClip_Win_Enum::ZDragClip_Win_Enum(ZDragClip_Win_DataObject* iDataObject, size_t iInitialIndex)
	{
	fTuple = *iDataObject->GetTuple();
	fCurrentIter = fTuple.begin();
	while (iInitialIndex && fCurrentIter != fTuple.end())
		{
		--iInitialIndex;
		++fCurrentIter;
		}
	ZThreadSafe_Set(fRefCount, 0);
	}

ZDragClip_Win_Enum::~ZDragClip_Win_Enum()
	{
	ZAssertStop(1, ZThreadSafe_Get(fRefCount) == 0);
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_Enum::QueryInterface(const IID& iInterfaceID, void** oObjectRef)
	{
	*oObjectRef = nil;
	if (iInterfaceID == IID_IUnknown || iInterfaceID == IID_IEnumFORMATETC)
		{
		*oObjectRef = static_cast<IEnumFORMATETC*>(this);
		this->AddRef();
		return NOERROR;
		}

	return E_NOINTERFACE;
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_Enum::AddRef()
	{
	return ZThreadSafe_IncReturnNew(fRefCount);
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_Enum::Release()
	{
	if (int newValue = ZThreadSafe_DecReturnNew(fRefCount))
		return newValue;
	delete this;
	return 0;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_Enum::Next(ULONG iRequestedCount, FORMATETC* oFORMATETC, ULONG* oActualCount)
	{
	ZAssertStop(1, oFORMATETC);

	size_t actualCount = 0;
	while (actualCount < iRequestedCount && fCurrentIter != fTuple.end())
		{
		CLIPFORMAT theCLIPFORMAT;
		string currentName = fTuple.NameOf(fCurrentIter);
		bool isString;
		if (ZDragClipManager::sGet()->LookupMIME(currentName, theCLIPFORMAT, isString))
			{
			oFORMATETC[actualCount].cfFormat = theCLIPFORMAT;
			oFORMATETC[actualCount].ptd = nil;
			oFORMATETC[actualCount].dwAspect = DVASPECT_CONTENT;
			oFORMATETC[actualCount].lindex = -1;
			oFORMATETC[actualCount].tymed = TYMED_HGLOBAL;
			++actualCount;
			}
		++fCurrentIter;
		}
	if (oActualCount)
		*oActualCount = actualCount;
	if (actualCount != iRequestedCount)
		return S_FALSE;
	return S_OK;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_Enum::Skip(ULONG iSkipCount)
	{
	size_t validCount = 0;
	while (validCount < iSkipCount && fCurrentIter != fTuple.end())
		{
		CLIPFORMAT theCLIPFORMAT;
		string currentName = fTuple.NameOf(fCurrentIter);
		bool isString;
		if (ZDragClipManager::sGet()->LookupMIME(currentName, theCLIPFORMAT, isString))
			++validCount;
		++fCurrentIter;
		}
	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_Enum::Reset()
	{
	fCurrentIter = fTuple.begin();
	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_Enum::Clone(IEnumFORMATETC** outIEnumFORMATETC)
	{
	*outIEnumFORMATETC = new ZDragClip_Win_Enum(fTuple, fCurrentIter);
	return NOERROR;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_DropSource

ZDragClip_Win_DropSource::ZDragClip_Win_DropSource()
	{
	ZThreadSafe_Set(fRefCount, 0);
	}

ZDragClip_Win_DropSource::~ZDragClip_Win_DropSource()
	{
	ZAssertStop(1, ZThreadSafe_Get(fRefCount) == 0);
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_DropSource::QueryInterface(const IID& inInterfaceID, void** outObjectRef)
	{
	*outObjectRef = nil;
	if (inInterfaceID == IID_IUnknown || inInterfaceID == IID_IDropSource)
		{
		*outObjectRef = static_cast<IDropSource*>(this);
		static_cast<IDropSource*>(this)->AddRef();
		return NOERROR;
		}
	return E_NOINTERFACE;
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_DropSource::AddRef()
	{
	return ZThreadSafe_IncReturnNew(fRefCount);
	}

ULONG STDMETHODCALLTYPE ZDragClip_Win_DropSource::Release()
	{
	if (int newValue = ZThreadSafe_DecReturnNew(fRefCount))
		return newValue;
	delete this;
	return 0;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_DropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
	{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;

	if (!(grfKeyState & MK_LBUTTON))
		return DRAGDROP_S_DROP;

	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZDragClip_Win_DropSource::GiveFeedback(DWORD dwEffect)
	{ return DRAGDROP_S_USEDEFAULTCURSORS; }

#endif // ZCONFIG(OS, Win32)

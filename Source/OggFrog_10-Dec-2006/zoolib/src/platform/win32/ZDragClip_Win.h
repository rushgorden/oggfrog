/*  @(#) $Id: ZDragClip_Win.h,v 1.8 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZDragClip_Win__
#define __ZDragClip_Win__ 1
#include "zconfig.h"

#if ZCONFIG(OS, Win32)

#include "ZDragClip.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_DataObject

class ZDragClip_Win_DataObject : public IDataObject
	{
public:
	static const IID sIID;

	ZDragClip_Win_DataObject(const ZTuple& iTuple);
	virtual ~ZDragClip_Win_DataObject();

// From IUnknown via IDataObject
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iInterfaceID, void** oObjectRef);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

// From IDataObject
	virtual HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium);
	virtual HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*, STGMEDIUM*);
	virtual HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC*);
	virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*, FORMATETC*);
	virtual HRESULT STDMETHODCALLTYPE SetData(FORMATETC*, STGMEDIUM*, BOOL);
	virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc);
	virtual HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*, DWORD, LPADVISESINK, DWORD *);
	virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD);
	virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(LPENUMSTATDATA *);

// Our protocol
	const ZTuple* GetTuple() const;

// Utility method
	static ZTuple sAsTuple(IDataObject* iIDataObject);

protected:
	ZThreadSafe_t fRefCount;
	ZTuple fTuple;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_Enum

class ZDragClip_Win_Enum : public IEnumFORMATETC
	{
	ZDragClip_Win_Enum(const ZTuple& iTuple, ZTuple::const_iterator iCurrentIter);
public:
	ZDragClip_Win_Enum(ZDragClip_Win_DataObject* iDataObject, size_t iInitialIndex);
	virtual ~ZDragClip_Win_Enum();

// From IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iInterfaceID, void** oObjectRef);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

// From IEnumFORMATETC
	virtual HRESULT STDMETHODCALLTYPE Next(ULONG iRequestedCount, FORMATETC* oFORMATETC, ULONG* oActualCount);
	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG iSkipCount);
	virtual HRESULT STDMETHODCALLTYPE Reset();
	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC** oIEnumFORMATETC);

protected:
	ZThreadSafe_t fRefCount;
	ZTuple fTuple;
	ZTuple::const_iterator fCurrentIter;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDragClip_Win_Drop

class ZDragClip_Win_DropSource : public IDropSource
	{
public:
	ZDragClip_Win_DropSource();
	virtual ~ZDragClip_Win_DropSource();

// From IDropSource
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iInterfaceID, void** oObjectRef);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	virtual HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

protected:
	ZThreadSafe_t fRefCount;
	};

#endif // ZCONFIG(OS, Win32)
#endif // __ZDragClip_Win__


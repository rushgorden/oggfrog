static const char rcsid[] = "@(#) $Id: ZOSWindow_Win.cpp,v 1.20 2006/04/14 21:02:35 agreen Exp $";

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

#include "ZOSWindow_Win.h"

#if ZCONFIG(API_OSWindow, Win32)
#include "ZDC_GDI.h"
#include "ZDragClip_Win.h"
#include "ZMenu_Win.h"
#include "ZThreadSimple.h"
#include "ZTicks.h"
#include "ZUnicode.h"
#include "ZUtil_STL.h"
#include "ZUtil_Win.h"

#include "ZCompat_algorithm.h" // for swap

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// ==================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_Win 2
#define kDebug_Capture 3
#define kDebug_Palette 3
#define kDebug_Drag 3

// =================================================================================================
#pragma mark -
#pragma mark * Static data

static UINT sRegisterWindowMessage(const UTF8* iMessage)
	{
	if (ZUtil_Win::sUseWAPI())
		{
		string16 theString16 = ZUnicode::sAsUTF16(iMessage);
		return ::RegisterWindowMessageW(theString16.c_str());
		}
	else
		{
		return ::RegisterWindowMessageA(iMessage);
		}
	}

static UINT sMSG_Destroy = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_Destroy");
static UINT sMSG_AcquireCapture = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_AcquireCapture");
static UINT sMSG_ReleaseCapture = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_ReleaseCapture");
static UINT sMSG_GetActive = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_GetActive");
static UINT sMSG_Menu = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_Menu");
static UINT sMSG_InstallMenu = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_InstallMenu");
static UINT sMSG_Show = ::sRegisterWindowMessage("ZOSWindow_Win::sShow");
static UINT sMSG_BringFront = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_BringFront");
static UINT sMSG_MDIMenuBarChanged = 
	::sRegisterWindowMessage("ZOSWindow_Win::sMSG_MDIMenuBarChanged");
static UINT sMSG_MDIActivated = ::sRegisterWindowMessage("ZOSWindow_Win::sMSG_MDIActivated");

static const UTF16 sWNDCLASSName_TaskBarDummyW[] = L"zoolib:TaskBarDummy";
static const char sWNDCLASSName_TaskBarDummyA[] = "zoolib:TaskBarDummy";

// =================================================================================================
#pragma mark -
#pragma mark * Utility Methods

static bool sAllowActivationChange = false;
static bool sDisallowActivationChange = false;

static bool sExpectingWindowPosChanging = false;
static bool MySetWindowPos(HWND hWnd,HWND hWndInsertAfter,int X,int Y,int cx,int cy,UINT uFlags)
	{
	bool oldExpectingWindowPosChanging = sExpectingWindowPosChanging;
	bool result = ::SetWindowPos(hWnd,hWndInsertAfter,X,Y,cx,cy,uFlags);
	sExpectingWindowPosChanging = oldExpectingWindowPosChanging;
	return result;
	}

static void sRegisterWindowClassW(const UTF16* inWNDCLASSName, WNDPROC inWNDPROC)
	{
	WNDCLASSW windowClass;
	if (!::GetClassInfoW(::GetModuleHandleW(nil), inWNDCLASSName, &windowClass) &&
		!::GetClassInfoW(nil, inWNDCLASSName, &windowClass))
		{
		windowClass.style = 0;
		windowClass.lpfnWndProc = inWNDPROC;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = ::GetModuleHandleW(nil);
		windowClass.hIcon = nil;
		windowClass.hCursor = nil;
		windowClass.hbrBackground = nil;
		windowClass.lpszMenuName = nil;
		windowClass.lpszClassName = inWNDCLASSName;
		ATOM theResult = ::RegisterClassW(&windowClass);
		ZAssertStop(kDebug_Win, theResult != 0);
		}
	}

static void sRegisterWindowClassA(const UTF8* inWNDCLASSName, WNDPROC inWNDPROC)
	{
	WNDCLASSA windowClass;
	if (!::GetClassInfoA(::GetModuleHandleA(nil), inWNDCLASSName, &windowClass) &&
		!::GetClassInfoA(nil, inWNDCLASSName, &windowClass))
		{
		windowClass.style = 0;
		windowClass.lpfnWndProc = inWNDPROC;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = ::GetModuleHandleA(nil);
		windowClass.hIcon = nil;
		windowClass.hCursor = nil;
		windowClass.hbrBackground = nil;
		windowClass.lpszMenuName = nil;
		windowClass.lpszClassName = inWNDCLASSName;
		ATOM theResult = ::RegisterClassA(&windowClass);
		ZAssertStop(kDebug_Win, theResult != 0);
		}
	}

static void sSetupIconInfo(const ZDCPixmap& inPixmap, const ZDCPixmap& inMask,
	ZCoord inWidth, ZCoord inHeight, bool inStretchToFit, ICONINFO& outICONINFO)
	{
	HDC dummyHDC = ::GetDC(nil);

	ZRef<ZDCPixmapRep_DIB> sourcePixmapRep_DIB
		= ZDCPixmapRep_DIB::sAsPixmapRep_DIB(inPixmap.GetRep());
	BITMAPINFO* sourceBMI;
	char* sourceBits;
	ZRect sourceBounds;
	sourcePixmapRep_DIB->GetDIBStuff(sourceBMI, sourceBits, &sourceBounds);

	// Create the real content bitmap
	HDC contentHDC = ::CreateCompatibleDC(dummyHDC);
	HBITMAP contentHBITMAP = ::CreateCompatibleBitmap(dummyHDC, inWidth, inHeight);
	HBITMAP contentOriginalHBITMAP = (HBITMAP)::SelectObject(contentHDC, contentHBITMAP);

	if (inStretchToFit)
		{
		::StretchDIBits(contentHDC, 0, 0, inWidth, inHeight,
						sourceBounds.left, sourceBounds.top,
						sourceBounds.Width(), sourceBounds.Height(),
						sourceBits, sourceBMI, DIB_RGB_COLORS, SRCCOPY);
		}
	else
		{
		if (inWidth > sourceBounds.Width() || inHeight > sourceBounds.Height())
			::PatBlt(contentHDC, 0, 0, inWidth, inHeight, WHITENESS);

		::StretchDIBits(contentHDC, 0, 0, sourceBounds.Width(), sourceBounds.Height(),
						sourceBounds.left, sourceBounds.top,
						sourceBounds.Width(), sourceBounds.Height(),
						sourceBits, sourceBMI, DIB_RGB_COLORS, SRCCOPY);
		}

	ZRef<ZDCPixmapRep_DIB> maskPixmapRep_DIB = ZDCPixmapRep_DIB::sAsPixmapRep_DIB(inMask.GetRep());
	BITMAPINFO* maskBMI;
	char* maskBits;
	ZRect maskBounds;
	maskPixmapRep_DIB->GetDIBStuff(maskBMI, maskBits, &maskBounds);

	// Create the real mask HBITMAP (which seems to have to be one bit deep)
	HDC realMaskHDC = ::CreateCompatibleDC(dummyHDC);
	HBITMAP realMaskHBITMAP = ::CreateBitmap(inWidth, inHeight, 1, 1, 0);
	HBITMAP realMaskOriginalHBITMAP = (HBITMAP)::SelectObject(realMaskHDC, realMaskHBITMAP);

	if (inStretchToFit)
		{
		::StretchDIBits(realMaskHDC, 0, 0, inWidth, inHeight,
						maskBounds.left, maskBounds.top, maskBounds.Width(), maskBounds.Height(),
						maskBits, maskBMI, DIB_RGB_COLORS, SRCCOPY);
		}
	else
		{
		if (inWidth > maskBounds.Width() || inHeight > maskBounds.Height())
			::PatBlt(realMaskHDC, 0, 0, inWidth, inHeight, BLACKNESS);

		::StretchDIBits(realMaskHDC, 0, 0, maskBounds.Width(), maskBounds.Height(),
						maskBounds.left, maskBounds.top, maskBounds.Width(), maskBounds.Height(),
						maskBits, maskBMI, DIB_RGB_COLORS, SRCCOPY);
		}

	// Invert the cursor by the mask.
	::BitBlt(contentHDC, 0, 0, inWidth, inHeight, realMaskHDC, 0, 0, 0x00990066); // ~(S^D)

	// Invert the mask
	::PatBlt(realMaskHDC, 0, 0, inWidth, inHeight, DSTINVERT);

	// Now contentHBITMAP has a modified copy of the source pixels, sized correctly and with black
	// everywhere the original mask had white. realMaskHBITMAP is one bit deep, sized correctly,
	// and is inverted from the original mask, so that window's SRCAND drawing of the cursor will
	// work correctly. We can now clean up our working storage.
	::SelectObject(realMaskHDC, realMaskOriginalHBITMAP);
	::DeleteDC(realMaskHDC);
	::SelectObject(contentHDC, contentOriginalHBITMAP);
	::DeleteDC(contentHDC);

	::ReleaseDC(nil, dummyHDC);

	outICONINFO.hbmMask = realMaskHBITMAP;
	outICONINFO.hbmColor = contentHBITMAP;
	}

// Okay, a workaround for another screwed Windows API. Windows does *not* take ownership of the
// cursor that is passed in. So when we switch cursors, if the extant one is not a stock cursor
// someone has to delete it or we leak GDI resources. I stumbled across an MSJ article that
// confirms it is safe to call delete on any stock object, although it does cause it to get
// deselected (in the case of cursors at least). So, we destroy the old cursor, and all should
// be well, unless it was allocated by some other app which is expecting it to persist -- here's
// where more Windows lore would be helpful.
static void sSafeSetCursor(HCURSOR inHCURSOR)
	{
	HCURSOR oldHCURSOR = ::SetCursor(inHCURSOR);
	if (oldHCURSOR != inHCURSOR)
		::DestroyCursor(oldHCURSOR);
	}

static LRESULT sSendMessage(HWND iHWND, UINT iMessage, WPARAM iWPARAM, LPARAM iLPARAM)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::SendMessageW(iHWND, iMessage, iWPARAM, iLPARAM);
	else
		return ::SendMessageA(iHWND, iMessage, iWPARAM, iLPARAM);
	}

static BOOL sPostMessage(HWND iHWND, UINT iMessage, WPARAM iWPARAM, LPARAM iLPARAM)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::PostMessageW(iHWND, iMessage, iWPARAM, iLPARAM);
	else
		return ::PostMessageA(iHWND, iMessage, iWPARAM, iLPARAM);
	}

static void sSetCursor(const ZCursor& inCursor)
	{
	if (inCursor.IsCustom())
		{
		ZPoint theHotSpot;
		ZDCPixmap colorPixmap, monoPixmap, maskPixmap;
		inCursor.Get(colorPixmap, monoPixmap, maskPixmap, theHotSpot);

		ICONINFO theICONINFO;
		theICONINFO.fIcon = false;
		theICONINFO.xHotspot = theHotSpot.h;
		theICONINFO.yHotspot = theHotSpot.v;

		ZAssertStop(kDebug_Win, maskPixmap && (colorPixmap || monoPixmap));

		// Figure out how big the cursor is going to have to be
		ZCoord cursorWidth = ::GetSystemMetrics(SM_CXCURSOR);
		ZCoord cursorHeight = ::GetSystemMetrics(SM_CYCURSOR);

		// Use the color or mono pixmap?
		ZDCPixmap sourcePixmap(colorPixmap ? colorPixmap : monoPixmap);

		::sSetupIconInfo(sourcePixmap, maskPixmap, cursorWidth, cursorHeight, false, theICONINFO);

		HCURSOR theHCURSOR = (HCURSOR)::CreateIconIndirect(&theICONINFO);

		::DeleteObject(theICONINFO.hbmMask);
		::DeleteObject(theICONINFO.hbmColor);
		::sSafeSetCursor(theHCURSOR);
		}
	else
		{
		WORD cursorID;
		switch (inCursor.GetCursorType())
			{
			case ZCursor::eCursorTypeIBeam: cursorID = IDC_IBEAM; break;
			case ZCursor::eCursorTypeCross: cursorID = IDC_CROSS; break;
			case ZCursor::eCursorTypePlus: cursorID = IDC_CROSS; break;
			case ZCursor::eCursorTypeWatch: cursorID = IDC_WAIT; break;
			case ZCursor::eCursorTypeArrow:
			default:
				cursorID = IDC_ARROW;
				break;
			}

		HCURSOR theHCURSOR;
		if (ZUtil_Win::sUseWAPI())
			theHCURSOR = ::LoadCursorW(nil, MAKEINTRESOURCEW(cursorID));
		else
			theHCURSOR =  ::LoadCursorA(nil, MAKEINTRESOURCEA(cursorID));

		::sSafeSetCursor(theHCURSOR);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win_DragObject

class ZOSWindow_Win_DragObject : public ZDragClip_Win_DataObject
	{
public:
	static const IID sIID;
	ZOSWindow_Win_DragObject(const ZTuple& inTuple,
							const ZOSWindow_Win_DragFeedbackCombo& inFeedbackCombo,
							ZDragSource* inDragSource);
	virtual ~ZOSWindow_Win_DragObject();

// From IUnknown via ZDragClip_Win_DataObject
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& inInterfaceID, void** outObjectRef);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

// Our protocol
	ZDragSource* GetDragSource();
	void GetFeedbackCombo(ZOSWindow_Win_DragFeedbackCombo& outFeedbackCombo);

protected:
	ZOSWindow_Win_DragFeedbackCombo fFeedbackCombo;
	ZDragSource* fDragSource;
	};

const IID ZOSWindow_Win_DragObject::sIID = // (a62005e0-a075-11d2-8f58-00c0f023db44)
	{
	0xa62005e0,
	0xa075,
	0x11d2,
	{ 0x8f, 0x58, 0x00, 0xc0, 0xf0, 0x23, 0xdb, 0x44}
	};

ZOSWindow_Win_DragObject::ZOSWindow_Win_DragObject(const ZTuple& inTuple,
							const ZOSWindow_Win_DragFeedbackCombo& inFeedbackCombo,
							ZDragSource* inDragSource)
:	ZDragClip_Win_DataObject(inTuple), fFeedbackCombo(inFeedbackCombo), fDragSource(inDragSource)
	{}

ZOSWindow_Win_DragObject::~ZOSWindow_Win_DragObject()
	{}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win_DragObject::QueryInterface(const IID& inInterfaceID,
	void** outObjectRef)
	{
	if (inInterfaceID == ZOSWindow_Win_DragObject::sIID)
		{
		*outObjectRef = this;
		this->AddRef();
		return NOERROR;
		}

	return ZDragClip_Win_DataObject::QueryInterface(inInterfaceID, outObjectRef);
	}

ULONG STDMETHODCALLTYPE ZOSWindow_Win_DragObject::AddRef()
	{ return ZDragClip_Win_DataObject::AddRef(); }

ULONG STDMETHODCALLTYPE ZOSWindow_Win_DragObject::Release()
	{ return ZDragClip_Win_DataObject::Release(); }

ZDragSource* ZOSWindow_Win_DragObject::GetDragSource()
	{ return fDragSource; }

void ZOSWindow_Win_DragObject::GetFeedbackCombo(ZOSWindow_Win_DragFeedbackCombo& outFeedbackCombo)
	{ outFeedbackCombo = fFeedbackCombo; }

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win::DropTarget

class ZOSWindow_Win::DropTarget : public IDropTarget
	{
public:
	DropTarget(ZOSWindow_Win* inOSWindow);
	virtual ~DropTarget();

// From IUnknown via IDropTarget
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& inInterfaceID, void** outObjectRef);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

// From IDropTarget
	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* inDataObject, DWORD inKeyState,
			POINTL inLocation, DWORD* outEffect);
	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD inKeyState, POINTL inLocation, DWORD* outEffect);
	virtual HRESULT STDMETHODCALLTYPE DragLeave();
	virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject* inDataObject, DWORD inKeyState,
			POINTL inLocation, DWORD* outEffect);

protected:
	ZThreadSafe_t fRefCount;
	ZOSWindow_Win* fOSWindow;
	IDataObject* fDraggedDataObject;
	POINTL fLastDragLocation;
	};

ZOSWindow_Win::DropTarget::DropTarget(ZOSWindow_Win* inOSWindow)
:	fOSWindow(inOSWindow), fDraggedDataObject(nil)
	{
	ZThreadSafe_Set(fRefCount, 0);
	this->AddRef();
	}

ZOSWindow_Win::DropTarget::~DropTarget()
	{
	ZAssertStop(kDebug_Drag, ZThreadSafe_Get(fRefCount) == 0);
	}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::QueryInterface(const IID& inInterfaceID,
	void** outObjectRef)
	{
	*outObjectRef = nil;

	if (inInterfaceID == IID_IUnknown || inInterfaceID == IID_IDropTarget)
		{
		*outObjectRef = static_cast<IDropTarget*>(this);
		static_cast<IDropTarget*>(this)->AddRef();
		return NOERROR;
		}
	return E_NOINTERFACE;
	}

ULONG STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::AddRef()
	{
	return ZThreadSafe_IncReturnNew(fRefCount);
	}

ULONG STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::Release()
	{
	if (int newValue = ZThreadSafe_DecReturnNew(fRefCount))
		return newValue;
	delete this;
	return 0;
	}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::DragEnter(IDataObject* inDataObject,
	DWORD inKeyState, POINTL inLocation, DWORD* outEffect)
	{
	*outEffect = DROPEFFECT_COPY;
	ZAssertStop(kDebug_Drag, fDraggedDataObject == nil);
	fDraggedDataObject = inDataObject;
	ZAssertStop(kDebug_Drag, fDraggedDataObject != nil);
	fDraggedDataObject->AddRef();

	fLastDragLocation = inLocation;

	fOSWindow->Internal_RecordDrag(true, fDraggedDataObject, inKeyState, inLocation, outEffect);

	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::DragOver(DWORD inKeyState,
	POINTL inLocation, DWORD* outEffect)
	{
	*outEffect = DROPEFFECT_COPY;
	ZAssertStop(kDebug_Drag, fDraggedDataObject != nil);
	fOSWindow->Internal_RecordDrag(true, fDraggedDataObject, inKeyState, inLocation, outEffect);
	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::DragLeave()
	{
	POINTL dummyLocation;
	dummyLocation.x = dummyLocation.y = 0;

	fOSWindow->Internal_RecordDrag(false, fDraggedDataObject, 0, dummyLocation, nil);
	ZAssertStop(kDebug_Drag, fDraggedDataObject != nil);
	fDraggedDataObject->Release();
	fDraggedDataObject = nil;

	return NOERROR;
	}

HRESULT STDMETHODCALLTYPE ZOSWindow_Win::DropTarget::Drop(IDataObject* inDataObject,
	DWORD inKeyState, POINTL inLocation, DWORD* outEffect)
	{
	ZAssertStop(kDebug_Drag, fDraggedDataObject != nil);
	fDraggedDataObject->Release();
	fDraggedDataObject = nil;

	*outEffect = DROPEFFECT_COPY;

	fOSWindow->Internal_RecordDrag(false, inDataObject, inKeyState, inLocation, outEffect);
	fOSWindow->Internal_RecordDrop(inDataObject, inKeyState, inLocation, outEffect);
	return NOERROR;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win::Drag

class ZOSWindow_Win::Drag : public ZDrag
	{
public:
	Drag(const ZTuple& inTuple, ZDragSource* inDragSource);
	virtual ~Drag();

// From ZDrag
	virtual size_t CountTuples() const;
	virtual ZTuple GetTuple(size_t inItemNumber) const;
	virtual ZDragSource* GetDragSource() const;
	virtual void GetNative(void* outNative);

protected:
	ZTuple fTuple;
	ZDragSource* fDragSource;
	};

ZOSWindow_Win::Drag::Drag(const ZTuple& inTuple, ZDragSource* inDragSource)
:	fTuple(inTuple), fDragSource(inDragSource)
	{}

ZOSWindow_Win::Drag::~Drag()
	{}

size_t ZOSWindow_Win::Drag::CountTuples() const
	{ return 1; }

ZTuple ZOSWindow_Win::Drag::GetTuple(size_t inItemNumber) const
	{
	ZAssertStop(kDebug_Drag, inItemNumber == 0);
	return fTuple;
	}

ZDragSource* ZOSWindow_Win::Drag::GetDragSource() const
	{ return fDragSource; }

void ZOSWindow_Win::Drag::GetNative(void* outNative)
	{
	ZUnimplemented();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win::Drop

class ZOSWindow_Win::Drop : public ZDrop
	{
public:
	Drop(const ZTuple& inTuple, ZDragSource* inDragSource);
	virtual ~Drop();

// From ZDrag
	virtual size_t CountTuples() const;
	virtual ZTuple GetTuple(size_t inItemNumber) const;
	virtual ZDragSource* GetDragSource() const;
	virtual void GetNative(void* outNative);

protected:
	ZTuple fTuple;
	ZDragSource* fDragSource;
	};

ZOSWindow_Win::Drop::Drop(const ZTuple& inTuple, ZDragSource* inDragSource)
:	fTuple(inTuple), fDragSource(inDragSource)
	{}

ZOSWindow_Win::Drop::~Drop()
	{}

size_t ZOSWindow_Win::Drop::CountTuples() const
	{ return 1; }

ZTuple ZOSWindow_Win::Drop::GetTuple(size_t inItemNumber) const
	{
	ZAssertStop(kDebug_Drag, inItemNumber == 0);
	return fTuple;
	}

ZDragSource* ZOSWindow_Win::Drop::GetDragSource() const
	{ return fDragSource; }

void ZOSWindow_Win::Drop::GetNative(void* outNative)
	{
	ZUnimplemented();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_GDI_Window

class ZDCCanvas_GDI_Window : public ZDCCanvas_GDI
	{
public:
	ZDCCanvas_GDI_Window(ZOSWindow_Win* inWindow, HWND inHWND, HPALETTE inHPALETTE,
			ZMutex* inMutexToCheck, ZMutex* inMutexToLock);
	virtual ~ZDCCanvas_GDI_Window();

// Our protocol
	void OSWindowDisposed();

// From ZRefCountedWithFinalization via ZDCCanvas_GDI
	virtual void Finalize();

// From ZDCCanvas via ZDCCanvas_GDI
	virtual void Scroll(ZDCState& inState, const ZRect& inRect, ZCoord inDeltaH, ZCoord inDeltaV);
	virtual void CopyFrom(ZDCState& inState, const ZPoint& inDestLocation,
		ZRef<ZDCCanvas> inSourceCanvas,
		const ZDCState& inSourceState, const ZRect& inSourceRect);

// From ZDCCanvas_GDI
	virtual ZDCRgn Internal_CalcClipRgn(const ZDCState& inState);

// Our protocol
	HDC GetHDC() { return fHDC; }
	void InvalClip();

protected:
	ZOSWindow_Win* fOSWindow;
	HWND fHWND;
	};

ZDCCanvas_GDI_Window::ZDCCanvas_GDI_Window(ZOSWindow_Win* inWindow, HWND inHWND, HPALETTE inHPALETTE,
	ZMutex* inMutexToCheck, ZMutex* inMutexToLock)
	{
	ZMutexLocker locker(sMutex_List);
	fOSWindow = inWindow;
	fHWND = inHWND;
	fHDC = ::GetDC(fHWND);
	fMutexToCheck = inMutexToCheck;
	fMutexToLock = inMutexToLock;
	this->Internal_Link();
	::SelectPalette(fHDC, inHPALETTE, true);
	int countChanged = ::RealizePalette(fHDC);
	}

ZDCCanvas_GDI_Window::~ZDCCanvas_GDI_Window()
	{
	ZAssertLocked(kDebug_Win, sMutex_List);
	if (fHWND)
		{
		::ReleaseDC(fHWND, fHDC);
		fHWND = nil;
		fHDC = nil;
		}
	fMutexToCheck = nil;
	fMutexToLock = nil;
	this->Internal_Unlink();
	}

void ZDCCanvas_GDI_Window::OSWindowDisposed()
	{
	::ReleaseDC(fHWND, fHDC);
	fHWND = nil;
	fHDC = nil;
	fOSWindow = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	}

void ZDCCanvas_GDI_Window::Finalize()
	{
	ZMutexLocker locker(sMutex_List);
	if (fOSWindow)
		{
		fOSWindow->FinalizeCanvas(this);
		}
	else
		{
		// If we don't have a window, fMutexToCheck and fMutexToLock must already be nil
		ZAssertStop(kDebug_Win, fMutexToCheck == nil && fMutexToLock == nil);
		this->FinalizationComplete();
		delete this;
		}
	}

void ZDCCanvas_GDI_Window::Scroll(ZDCState& inState, const ZRect& inRect,
	ZCoord hDelta, ZCoord vDelta)
	{
	SetupDC theSetupDC(this, inState);
	RECT scrollRect = inRect + inState.fOrigin;
	ZDCRgn invalidRgn;
	::ScrollDC(fHDC, hDelta, vDelta, &scrollRect, &scrollRect, invalidRgn.GetHRGN(), nil);
	fOSWindow->Internal_RecordInvalidation(invalidRgn);
	}

void ZDCCanvas_GDI_Window::CopyFrom(ZDCState& inState, const ZPoint& inDestLocation,
	ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
	ZRef<ZDCCanvas_GDI> sourceCanvasGDI = ZRefDynamicCast<ZDCCanvas_GDI>(inSourceCanvas);

	ZAssertStop(kDebug_Win, sourceCanvasGDI);

	if (fHDC == nil || sourceCanvasGDI->Internal_GetHDC() == nil)
		return;

	ZRect destRect = inSourceRect + (inState.fOrigin + inDestLocation - inSourceRect.TopLeft());
	ZRect sourceRect = inSourceRect + inSourceState.fOrigin;

	if (sourceCanvasGDI == this)
		{
		// We're copying within the window. In order to atomically move pixels and be notified of
		// any region that was inaccesible because it's offscreen or obscured by another window
		// we use ScrollDC rather than BitBlt. Additionally we have to respect and update the
		// window's inval rgn.
		ZMutexLocker locker(*fMutexToLock);

		if (!fOriginValid)
			{
			fOriginValid = true;
			if (this->IsPrinting())
				{
				::SetMapMode(fHDC, MM_ANISOTROPIC);
				int deviceHRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
				int deviceVRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);
				::SetViewportExtEx(fHDC, deviceHRes, deviceVRes, nil);
				::SetWindowExtEx(fHDC, 72, 72, nil);
				}
			else
				{
				::SetMapMode(fHDC, MM_TEXT);
				}

			::SetWindowOrgEx(fHDC, 0, 0, nil);
			}

		// Get the region that we know cannot be copied from, because it's marked
		// as invalid or is part of the window's pseudo-structure (size box).
		ZDCRgn excludeRgn = fOSWindow->Internal_GetExcludeRgn();

		// destRgn is the set of pixels we want and are allowed to draw to, the
		// intersection of the current clip and the destRect less the exclude region
		ZDCRgn destRgn = ((inState.fClip + inState.fClipOrigin) & destRect) - excludeRgn;

		// sourceRgn is destRgn shifted back to the source location,
		// with the exclude region removed (again).
		ZDCRgn sourceRgn = (destRgn + (sourceRect.TopLeft() - destRect.TopLeft())) - excludeRgn;

		// drawnRgn is the subset of destRgn that has accessible corresponding pixels in the source.
		ZDCRgn drawnRgn = destRgn & (sourceRgn + (destRect.TopLeft() - sourceRect.TopLeft()));

		// invalidRgn is therefore the subset of destRgn that
		// does not have accessible pixels in the source.
		ZDCRgn invalidRgn = destRgn - drawnRgn;

		++fChangeCount_Clip;

		// Windows does not copy from pixels that lie outside of the current clip,
		// so we have to set the clip to be the union of drawnRgn and sourceRgn
		ZDCRgn theClipRgn = drawnRgn | sourceRgn;
		if (this->IsPrinting())
			{
			int deviceHRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
			int deviceVRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);
			float hScale = (float)deviceHRes / 72.0f;
			float vScale = (float)deviceVRes / 72.0f;
			theClipRgn.MakeScale(hScale, 0.0f, 0.0f, vScale, 0.0f, 0.0f);
			}
		::SelectClipRgn(fHDC, theClipRgn.GetHRGN());

		// The rect we're scrolling is the minimal bounding rectangle of the sourceRgn and destRgn.
		RECT gdiScrollRect = sourceRgn.Bounds() | destRgn.Bounds();
		ZDCRgn additionalInvalidRgn;
		::ScrollDC(fHDC, destRgn.Bounds().left - sourceRgn.Bounds().left,
			destRgn.Bounds().top - sourceRgn.Bounds().top,
			&gdiScrollRect, &gdiScrollRect, additionalInvalidRgn.GetHRGN(), nil);

		// additionalInvalidRgn marks all those pixels that (1) could not be drawn because they
		// were inaccesible due to overlying windows or offscreen (2) should have been copied
		// from outside the clip rgn, but weren't because they were outside the clip rgn.
		// (2) is sourceRgn - destRgn, ie all those pixels in the source that were not also in
		// the destination. We don't want to draw them in any event, so we remove them from
		// additionalInvalidRgn, leaving just those pixels which are in the destRgn but could
		// not be copied because of overlying windows.
		additionalInvalidRgn -= sourceRgn - destRgn;

		// Record both the pixels we know we were unable to copy (because they were
		// invalid in the source) plus those that ScrollDC told us about.
		fOSWindow->Internal_RecordInvalidation(invalidRgn | additionalInvalidRgn);
		}
	else
		{
		// We're copying to the window from some other canvas. SetupDC will clip out any
		// destination pixels that are already marked as invalid. If the source canvas is
		// some other window then tough -- it's generally not necessary to copy from one
		// window to another, so accounting for the invalid pixels in a source window is
		// not necessary.
		SetupDC theSetupDC(this, inState);
		SetupDC theSetupDCSource(sourceCanvasGDI.GetObject(), inSourceState);

		::BitBlt(fHDC, destRect.left, destRect.top, destRect.Width(), destRect.Height(),
							sourceCanvasGDI->Internal_GetHDC(),
							sourceRect.left, sourceRect.top, SRCCOPY);
		}
	}

ZDCRgn ZDCCanvas_GDI_Window::Internal_CalcClipRgn(const ZDCState& inState)
	{
	return (inState.fClip + inState.fClipOrigin) - fOSWindow->Internal_GetExcludeRgn();
	}

void ZDCCanvas_GDI_Window::InvalClip()
	{
	ZAssertStop(kDebug_Win, fMutexToLock->IsLocked());
	++fChangeCount_Clip;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win

ZOSWindow_Win::ZOSWindow_Win(bool inHasSizeBox, bool inHasResizeBorder, bool inHasCloseBox,
	bool inHasMenuBar, int32 inZLevel)
:	fCursor(ZCursor::eCursorTypeArrow)
	{
	fHasSizeBox = inHasSizeBox;
	fHasResizeBorder = inHasResizeBorder;
	fHasCloseBox = inHasCloseBox;

	fHasMenuBar = inHasMenuBar;
	fZLevel = inZLevel;

	fCanvas = nil;
	fHWND = nil;
	fCheckMouseOnTimer = false;
	fContainedMouse = false;
	fRequestedCapture = false;
	fDropTarget = nil;

	fSize_Min.h = 20;
	fSize_Min.v = 20;
	fSize_Max.h = 20000;
	fSize_Max.v = 20000;

	fDrag_RecordedContained = false;
	fDrag_ReportedContained = false;
	fDrag_DragSource = nil;
	fDrag_PostedMessage = false;

	fWindowProcFilter_Head = nil;

	fHPALETTE = ::CreateHalftonePalette(nil);
	}

ZOSWindow_Win::~ZOSWindow_Win()
	{
	ZAssertStop(kDebug_Win, fWindowProcFilter_Head == nil);
	// By the time we get here we *must* have destroyed our window -- remember that
	// CallBaseWindowProc is a pure virtual function and is implemented in our *derived classes*.
	ZAssertStop(kDebug_Win, fHWND == nil);
	::DeleteObject(fHPALETTE);
	}

bool ZOSWindow_Win::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow_Std::DispatchMessage(inMessage))
		return true;

	bool handledIt = false;
	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:DragPending")
			{
			ZMutexLocker locker(fMutex_Structure);

			fDrag_PostedMessage = false;

			this->Internal_UpdateDragFeedback();

			ZOSWindow_Win::Drag theDrag(fDrag_Tuple, fDrag_DragSource);
			fDrag_Tuple = ZTuple();
			fDrag_DragSource = nil;
			ZPoint theLocation = fDrag_ReportedLocation;

			ZMouse::Motion theMotion = ZMouse::eMotionMove;
			if (fDrag_ReportedContained != fDrag_RecordedContained)
				{
				fDrag_ReportedContained = fDrag_RecordedContained;
				theMotion = fDrag_ReportedContained ? ZMouse::eMotionEnter : ZMouse::eMotionLeave;
				}
			locker.Release();
			if (fOwner)
				fOwner->OwnerDrag(theLocation, theMotion, theDrag);
			return true;
			}
		else if (theWhat == "zoolib:Drop")
			{
			ZPoint theLocation = inMessage.GetPoint("where");
			ZDragSource* theDragSource
				= static_cast<ZDragSource*>(inMessage.GetPointer("dragSource"));
			ZTuple* theTuple = static_cast<ZTuple*>(inMessage.GetPointer("tuple"));
			if (fOwner)
				{
				ZOSWindow_Win::Drop theDrop(*theTuple, theDragSource);
				fOwner->OwnerDrop(theLocation, theDrop);
				}
			delete theTuple;
			return true;
			}
		}
	return false;
	}

void ZOSWindow_Win::SetShownState(ZShownState inState)
	{
	ASSERTLOCKED();
	::sSendMessage(fHWND, sMSG_Show, inState, 0);
	this->Internal_ReportShownStateChange();
	this->Internal_ReportActiveChange();
	}

void ZOSWindow_Win::SetTitle(const string& inTitle)
	{
	ASSERTLOCKED();
	if (ZUtil_Win::sUseWAPI())
		{
		::SetWindowTextW(fHWND, ZUnicode::sAsUTF16(inTitle).c_str());
		}
	else
		{
//		::SetWindowTextA(fHWND, ZUtil_Win::sUTF8ToWinCP(inTitle).c_str());
		::SetWindowTextA(fHWND, inTitle.c_str());
		}
	}

string ZOSWindow_Win::GetTitle()
	{
	ASSERTLOCKED();
	if (ZUtil_Win::sUseWAPI())
		{
		UTF16 tempTitle[256];
		::GetWindowTextW(fHWND, tempTitle, 256);
		return ZUnicode::sAsUTF8(tempTitle);
		}
	else
		{
		char tempTitle[256];
		::GetWindowTextA(fHWND, tempTitle, 256);
//		return ZUtil_Win::sWinCPToUTF8(tempTitle);
		return tempTitle;
		}
	}

void ZOSWindow_Win::SetTitleIcon(const ZDCPixmap& inColorPixmap,
	const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap)
	{
	ASSERTLOCKED();
	if ((inColorPixmap || inMonoPixmap) && inMaskPixmap)
		{
		// Use the color or mono pixmap?
		ZDCPixmap sourcePixmap(inColorPixmap ? inColorPixmap : inMonoPixmap);

		ICONINFO theICONINFO;
		theICONINFO.fIcon = true;

		// We could use the system metrics to determine how big the icon should be, but if we
		// do that then we're stuck with one size, even after the user changes the title bar
		// height. Instead we'll create an HICON that is the same size as our pixmap, and let
		// the OS resize it on the fly.
		// ZCoord theSize = ::GetSystemMetrics(SM_CYSIZE);

		::sSetupIconInfo(sourcePixmap, inMaskPixmap,
			sourcePixmap.Width(), sourcePixmap.Height(), false, theICONINFO);

		HICON newHICON = (HICON)::CreateIconIndirect(&theICONINFO);

		::DeleteObject(theICONINFO.hbmMask);
		::DeleteObject(theICONINFO.hbmColor);

		if (HICON oldHICON = reinterpret_cast<HICON>(::sSendMessage(fHWND, WM_SETICON, true,
			reinterpret_cast<LPARAM>(newHICON))))
			{
			::DeleteObject(oldHICON);
			}
		}
	}

ZCoord ZOSWindow_Win::GetTitleIconPreferredHeight()
	{
	ASSERTLOCKED();
	return ::GetSystemMetrics(SM_CYSIZE);
	}

void ZOSWindow_Win::SetCursor(const ZCursor& inCursor)
	{
	ASSERTLOCKED();
	fCursor = inCursor;
	if (fContainedMouse)
		::sSetCursor(inCursor);
	}

void ZOSWindow_Win::SetLocation(ZPoint inLocation)
	{
	ASSERTLOCKED();

	ZPoint theTopLeftInset, theBottomRightInset;
	sCalcContentInsets(fHWND, theTopLeftInset, theBottomRightInset);
	::MySetWindowPos(fHWND, nil,
		inLocation.h + theTopLeftInset.h,
		inLocation.v + theTopLeftInset.v,
		0, 0,
		SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Win::SetSize(ZPoint inSize)
	{
	ASSERTLOCKED();

	ZPoint theTopLeftInset, theBottomRightInset;
	sCalcContentInsets(fHWND, theTopLeftInset, theBottomRightInset);
	::MySetWindowPos(fHWND, nil,
		0, 0,
		inSize.h + theTopLeftInset.h - theBottomRightInset.h,
		inSize.v + theTopLeftInset.v - theBottomRightInset.v,
		SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Win::SetFrame(const ZRect& inFrame)
	{
	ASSERTLOCKED();
	RECT oldClientRECT;
	::GetClientRect(fHWND, &oldClientRECT);
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&oldClientRECT));
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&oldClientRECT) + 1);
	ZRect oldClientRect(oldClientRECT);

	RECT oldWindowRECT;
	::GetWindowRect(fHWND, &oldWindowRECT);
	ZRect oldWindowRect(oldWindowRECT);

	ZRect newWindowRect;
	newWindowRect.left = inFrame.left + oldWindowRect.left - oldClientRect.left;
	newWindowRect.right = inFrame.right + oldWindowRect.right - oldClientRect.right;
	newWindowRect.top = inFrame.top + oldWindowRect.top - oldClientRect.top;
	newWindowRect.bottom = inFrame.bottom + oldWindowRect.bottom - oldClientRect.bottom;

	UINT theFlags = SWP_NOZORDER | SWP_NOACTIVATE;

	if (newWindowRect.TopLeft() == oldWindowRect.TopLeft())
		{
		if (newWindowRect.Size() == oldWindowRect.Size())
			return;
		theFlags |= SWP_NOMOVE;
		}
	else
		{
		if (newWindowRect.Size() == oldWindowRect.Size())
			theFlags |= SWP_NOSIZE;
		else
			{
			// If we're changing both size & location, don't do any bits preservation
			theFlags |= SWP_NOCOPYBITS;
			}
		}

	if (newWindowRect.Size() != oldWindowRect.Size()
		&& newWindowRect.TopLeft() != oldWindowRect.TopLeft())
		{
		theFlags |= SWP_NOCOPYBITS;
		}

	if (newWindowRect.TopLeft() == oldWindowRect.TopLeft())
		theFlags |= SWP_NOMOVE;

	::MySetWindowPos(fHWND, nil,
		newWindowRect.left, newWindowRect.top,
		newWindowRect.Width(), newWindowRect.Height(),
		theFlags);

	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Win::SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize)
	{
	ASSERTLOCKED();
	// Force into reasonable ranges
	if (inMinSize.h <= 0)
		inMinSize.h = 1;
	if (inMinSize.v <= 0)
		inMinSize.v = 1;
	if (inMaxSize.h < inMinSize.h)
		inMaxSize.h = inMinSize.h;
	if (inMaxSize.v < inMinSize.v)
		inMaxSize.v = inMinSize.v;

	// Record the limits
	fSize_Min = inMinSize;
	fSize_Max = inMaxSize;

	// Tell ourselves to resize to our current size. If our current size falls
	// outside of the acceptable range, then it will get coerced appropriately.
	this->SetSize(this->GetSize());
	}

void ZOSWindow_Win::BringFront(bool inMakeVisible)
	{
	ASSERTLOCKED();
	::sSendMessage(fHWND, sMSG_BringFront, 0, 0);
	this->Internal_ReportShownStateChange();
	this->Internal_ReportActiveChange();
	}

void ZOSWindow_Win::GetContentInsets(ZPoint& topLeftInset, ZPoint& bottomRightInset)
	{
	ASSERTLOCKED();
	sCalcContentInsets(fHWND, topLeftInset, bottomRightInset);
	}

bool ZOSWindow_Win::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();
	if (fHasSizeBox)
		{
		outSizeBoxSize = fSizeBoxFrame.Size();
		return true;
		}
	return false;
	}

ZDCRgn ZOSWindow_Win::GetVisibleContentRgn()
	{
	ASSERTLOCKED();
	return ZDCRgn(this->GetSize());
	}

void ZOSWindow_Win::InvalidateRegion(const ZDCRgn& inBadRgn)
	{
	ASSERTLOCKED();
	if (::IsWindowVisible(fHWND))
		{
		ZMutexLocker lockerStructure(fMutex_Structure);
		if (fCanvas)
			fCanvas->InvalClip();
		this->Internal_RecordInvalidation(inBadRgn);
		}
	}

void ZOSWindow_Win::UpdateNow()
	{
	ASSERTLOCKED();
	this->Internal_ReportInvalidation();
	}

ZRef<ZDCCanvas> ZOSWindow_Win::GetCanvas()
	{
	ASSERTLOCKED();

	ZMutexLocker lockerStructure(fMutex_Structure);
	if (!fCanvas)
		{
		fCanvas = new ZDCCanvas_GDI_Window(this, fHWND, fHPALETTE,
			&fMutex_MessageDispatch, &fMutex_Structure);
		}
	return fCanvas;
	}

void ZOSWindow_Win::AcquireCapture()
	{
	ASSERTLOCKED();
	::sSendMessage(fHWND, sMSG_AcquireCapture, 0, 0);
	}

void ZOSWindow_Win::ReleaseCapture()
	{
	ASSERTLOCKED();
	::sSendMessage(fHWND, sMSG_ReleaseCapture, 0, 0);
	}

void ZOSWindow_Win::GetNative(void* outNative)
	{
	*reinterpret_cast<HWND*>(outNative) = fHWND;
	}

ZDragInitiator* ZOSWindow_Win::CreateDragInitiator()
	{
	ASSERTLOCKED();
	return ZOSApp_Win::sGet()->CreateDragInitiator();
	}

ZRect ZOSWindow_Win::GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly)
	{
	ZRect theRect(ZRect::sZero);
	if (inScreenIndex == 0)
		{
		if (inUsableSpaceOnly)
			{
			RECT workArea;
			if (ZUtil_Win::sUseWAPI())
				::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
			else
				::SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
			theRect = workArea;
			}
		else
			{
			theRect.right = ::GetSystemMetrics(SM_CXSCREEN);
			theRect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
			}
		}
	return theRect;
	}

void ZOSWindow_Win::Internal_Update(const ZDCRgn& inUpdateRgn)
	{
	if (fDrag_ReportedFeedback.fPixmap || fDrag_ReportedFeedback.fRgn)
		{
		ZDC theDC(this->GetCanvas(), ZPoint::sZero);
		theDC.SetInk(ZDCInk());
		theDC.SetClip(inUpdateRgn);

		if (fOwner)
			fOwner->OwnerUpdate(theDC);

		ZPoint theLocation = (fDrag_ReportedLocation - fDrag_ReportedFeedback.fHotPoint);
		if (fDrag_ReportedFeedback.fRgn)
			{
			theDC.SetInk(ZDCInk::sGray);
			theDC.SetMode(ZDC::modeXor);
			theDC.Fill(fDrag_ReportedFeedback.fRgn + theLocation);
			}

		if (fDrag_ReportedFeedback.fPixmap)
			{
			if (fDrag_ReportedFeedback.fMaskPixmap)
				{
				theDC.Draw(theLocation,
					fDrag_ReportedFeedback.fPixmap, fDrag_ReportedFeedback.fMaskPixmap);
				}
			else
				{
				if (fDrag_ReportedFeedback.fMaskRgn)
					theDC.SetClip(theDC.GetClip() & (fDrag_ReportedFeedback.fMaskRgn + theLocation));
				theDC.Draw(theLocation, fDrag_ReportedFeedback.fPixmap);
				}
			}
		}
	else
		{
		if (fOwner)
			{
			ZDC theDC(this->GetCanvas(), ZPoint::sZero);
			theDC.SetInk(ZDCInk());
			theDC.SetClip(inUpdateRgn);
			fOwner->OwnerUpdate(theDC);
			}
		}
	}

ZDCRgn ZOSWindow_Win::Internal_GetExcludeRgn()
	{
	ZAssertLocked(kDebug_Win, fMutex_Structure);
	ZDCRgn theRgn = this->Internal_GetInvalidRgn();
	if (fHasSizeBox)
		theRgn |= fSizeBoxFrame;
	return theRgn;
	}

void ZOSWindow_Win::Internal_TitleIconClick(const ZMessage& inMessage)
	{
	ASSERTLOCKED();
	::sPostMessage(fHWND, sMSG_Menu, SC_MOUSEMENU, inMessage.GetInt32("LPARAM"));
	}

void ZOSWindow_Win::FinalizeCanvas(ZDCCanvas_GDI_Window* inCanvas)
	{
	ZMutexLocker locker(fMutex_Structure);

	ZAssertStop(kDebug_Win, inCanvas == fCanvas);

	if (fCanvas->GetRefCount() > 1)
		fCanvas->FinalizationComplete();
	else
		{
		fCanvas = nil;
		inCanvas->FinalizationComplete();
		delete inCanvas;
		}
	}

ZOSWindow_Win* ZOSWindow_Win::sFromHWNDNilOkayW(HWND inHWND)
	{
	// Get the process ID of the window
	DWORD windowProcessID;
	::GetWindowThreadProcessId(inHWND, &windowProcessID);
	// And our own process ID
	DWORD currentProcessID = ::GetCurrentProcessId();
	if (windowProcessID == currentProcessID)
		{
		// This is an HWND in this process. Check for the marker property.
		if (::GetPropW(inHWND, L"ZOSWindow_Win"))
			{
			if (ZOSWindow_Win* theWindow
				= reinterpret_cast<ZOSWindow_Win*>(::GetWindowLongW(inHWND, GWL_USERDATA)))
				{
				ZAssertStop(kDebug_Win, theWindow->fHWND == inHWND);
				return theWindow;
				}
			}
		}
	return nil;
	}

ZOSWindow_Win* ZOSWindow_Win::sFromHWNDNilOkayA(HWND inHWND)
	{
	// Get the process ID of the window
	DWORD windowProcessID;
	::GetWindowThreadProcessId(inHWND, &windowProcessID);
	// And our own process ID
	DWORD currentProcessID = ::GetCurrentProcessId();
	if (windowProcessID == currentProcessID)
		{
		// This is an HWND in this process. Check for the marker property.
		if (::GetPropA(inHWND, "ZOSWindow_Win"))
			{
			if (ZOSWindow_Win* theWindow
				= reinterpret_cast<ZOSWindow_Win*>(::GetWindowLongA(inHWND, GWL_USERDATA)))
				{
				ZAssertStop(kDebug_Win, theWindow->fHWND == inHWND);
				return theWindow;
				}
			}
		}
	return nil;
	}

ZOSWindow_Win* ZOSWindow_Win::sFromHWNDNilOkay(HWND inHWND)
	{
	if (ZUtil_Win::sUseWAPI())
		return sFromHWNDNilOkayW(inHWND);
	else
		return sFromHWNDNilOkayA(inHWND);
	}

ZOSWindow_Win* ZOSWindow_Win::sFromHWNDW(HWND inHWND)
	{
#if ZCONFIG_Debug
	// Require that the HWND is in our process
	DWORD theProcessID;
	::GetWindowThreadProcessId(inHWND, &theProcessID);
	ZAssertStop(kDebug_Win, theProcessID == ::GetCurrentProcessId());
#endif // ZCONFIG_Debug

	ZOSWindow_Win* theWindow
		= reinterpret_cast<ZOSWindow_Win*>(::GetWindowLongW(inHWND, GWL_USERDATA));

	ZAssertStop(kDebug_Win, theWindow);
	ZAssertStop(kDebug_Win, theWindow->fHWND == inHWND);
	return theWindow;
	}

ZOSWindow_Win* ZOSWindow_Win::sFromHWNDA(HWND inHWND)
	{
#if ZCONFIG_Debug
	// Require that the HWND is in our process
	DWORD theProcessID;
	::GetWindowThreadProcessId(inHWND, &theProcessID);
	ZAssertStop(kDebug_Win, theProcessID == ::GetCurrentProcessId());
#endif // ZCONFIG_Debug

	ZOSWindow_Win* theWindow
		= reinterpret_cast<ZOSWindow_Win*>(::GetWindowLongA(inHWND, GWL_USERDATA));

	ZAssertStop(kDebug_Win, theWindow);
	ZAssertStop(kDebug_Win, theWindow->fHWND == inHWND);
	return theWindow;
	}

/*ZOSWindow_Win* ZOSWindow_Win::sFromHWND(HWND inHWND)
	{
	if (ZUtil_Win::sUseWAPI())
		return sFromHWNDW(inHWND);
	else
		return sFromHWNDA(inHWND);
	}*/

void ZOSWindow_Win::Internal_RecordDrag(bool inIsContained, IDataObject* inDataObject,
	DWORD inKeyState, POINTL inLocation, DWORD* outEffect)
	{
	ZMutexLocker locker(fMutex_Structure);

	POINT windowLocation;
	windowLocation.x = inLocation.x;
	windowLocation.y = inLocation.y;
	::ScreenToClient(fHWND, &windowLocation);

	fDrag_RecordedLocation = windowLocation;
	fDrag_RecordedContained = inIsContained;

	fDrag_Tuple = ZDragClip_Win_DataObject::sAsTuple(inDataObject);

	ZOSWindow_Win_DragObject* theDragObject = nil;
	if (SUCCEEDED(inDataObject->
		QueryInterface(ZOSWindow_Win_DragObject::sIID, (void**)&theDragObject)) && theDragObject)
		{
		fDrag_DragSource = theDragObject->GetDragSource();
		if (inIsContained)
			theDragObject->GetFeedbackCombo(fDrag_RecordedFeedback);
		else
			fDrag_RecordedFeedback = ZOSWindow_Win_DragFeedbackCombo();
		theDragObject->Release();
		}
	else
		{
		fDrag_DragSource = nil;
		fDrag_RecordedFeedback = ZOSWindow_Win_DragFeedbackCombo();
		}

	if (!fDrag_PostedMessage)
		{
		fDrag_PostedMessage = true;
		ZMessage theWakeupMessage;
		theWakeupMessage.SetString("what", "zoolib:DragPending");
		this->QueueMessage(theWakeupMessage);
		}
	}

void ZOSWindow_Win::Internal_RecordDrop(IDataObject* inDataObject, DWORD inKeyState,
	POINTL inLocation, DWORD* outEffect)
	{
	ZMutexLocker locker(fMutex_Structure);

	POINT windowLocationPOINT;
	windowLocationPOINT.x = inLocation.x;
	windowLocationPOINT.y = inLocation.y;
	::ScreenToClient(fHWND, &windowLocationPOINT);
	ZPointPOD windowLocation;
	windowLocation.h = windowLocationPOINT.x;
	windowLocation.v = windowLocationPOINT.y;

	ZDragSource* theDragSource = nil;
	ZOSWindow_Win_DragObject* theDragObject = nil;
	if (SUCCEEDED(inDataObject->
		QueryInterface(ZOSWindow_Win_DragObject::sIID, (void**)&theDragObject)) && theDragObject)
		{
		theDragSource = theDragObject->GetDragSource();
		theDragObject->Release();
		}

	ZMessage theDropMessage;
	theDropMessage.SetString("what", "zoolib:Drop");
	theDropMessage.SetPoint("where", windowLocation);
	theDropMessage.SetPointer("dragSource", theDragSource);
	theDropMessage.SetPointer("tuple", new ZTuple(ZDragClip_Win_DataObject::sAsTuple(inDataObject)));
	this->QueueMessage(theDropMessage);
	}

void ZOSWindow_Win::Internal_UpdateDragFeedback()
	{
	ZAssertLocked(kDebug_Drag, fMutex_Structure);
	ZDebugLogf(kDebug_Drag, ("Internal_SetDragFeedback Begin update %X", ZThread::sCurrentID()));

	// Diff the old and new regions, to redraw only the changed region.
	ZDCRgn oldPixmapBoundsRegion = ZRect(fDrag_ReportedFeedback.fPixmap.Size());
	oldPixmapBoundsRegion |= ZRect(fDrag_ReportedFeedback.fMaskPixmap.Size());
	oldPixmapBoundsRegion += (fDrag_ReportedLocation - fDrag_ReportedFeedback.fHotPoint);

	ZDCRgn newPixmapBoundsRegion = ZRect(fDrag_RecordedFeedback.fPixmap.Size());
	newPixmapBoundsRegion |= ZRect(fDrag_RecordedFeedback.fMaskPixmap.Size());
	newPixmapBoundsRegion += (fDrag_RecordedLocation - fDrag_RecordedFeedback.fHotPoint);

	ZDCRgn oldRegion = fDrag_ReportedFeedback.fRgn
		+ (fDrag_ReportedLocation - fDrag_ReportedFeedback.fHotPoint);
	oldRegion -= (oldPixmapBoundsRegion | newPixmapBoundsRegion);

	ZDCRgn newRegion = fDrag_RecordedFeedback.fRgn
		+ (fDrag_RecordedLocation - fDrag_RecordedFeedback.fHotPoint);
	newRegion -= (oldPixmapBoundsRegion | newPixmapBoundsRegion);

	{
	ZDC theDC(this->GetCanvas(), ZPoint::sZero);
	theDC.SetClip(this->GetVisibleContentRgn());
	theDC.SetInk(ZDCInk::sGray);
	theDC.SetMode(ZDC::modeXor);
	theDC.Fill(newRegion ^ oldRegion);
	}

	// Update our instance variables
	fDrag_ReportedLocation = fDrag_RecordedLocation;
	fDrag_ReportedFeedback = fDrag_RecordedFeedback;

	// *Invalidate* the regions occupied by the old and new pixmaps
	this->InvalidateRegion(oldPixmapBoundsRegion | newPixmapBoundsRegion);
	ZDebugLogf(kDebug_Drag, ("Internal_SetDragFeedback Pre update %X", ZThread::sCurrentID()));
	this->UpdateNow();
	ZDebugLogf(kDebug_Drag, ("Internal_SetDragFeedback Post update %X", ZThread::sCurrentID()));
	}

void ZOSWindow_Win::sCalcContentInsets(HWND inHWND,
	ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	// Subtracting topLeftInset from a window's content rect top left will give the
	// top left of the window's structure rect.
	// Subtracting bottomRightInset from a window's content rect bottom right will
	// give the bottom right of the window's structure rect.
	// So topLeftInset will generally have positive coordinates, and bottomRightInset
	// will have negative coordinates.
	RECT clientRect;
	::GetClientRect(inHWND, &clientRect);

	POINT localTL;
	localTL.x = clientRect.left;
	localTL.y = clientRect.top;

	POINT localBR;
	localBR.x = clientRect.right;
	localBR.y = clientRect.bottom;

// Work this out in screen coords
	::ClientToScreen(inHWND, &localTL);
	::ClientToScreen(inHWND, &localBR);

	outTopLeftInset = localTL;
	outBottomRightInset = localBR;

	RECT windowRect;
	::GetWindowRect(inHWND, &windowRect);

	outTopLeftInset.h -= windowRect.left;
	outTopLeftInset.v -= windowRect.top;
	outBottomRightInset.h -= windowRect.right;
	outBottomRightInset.v -= windowRect.bottom;
	}

void ZOSWindow_Win::sFromCreationAttributes(const CreationAttributes& inAttributes,
	DWORD& outWindowStyle, DWORD& outExWindowStyle)
	{
	outWindowStyle = 0;
	outExWindowStyle = 0;
	switch (inAttributes.fLook)
		{
		case ZOSWindow::lookDocument:
			{
			outWindowStyle |= WS_POPUP | WS_BORDER | WS_DLGFRAME;
			if (inAttributes.fHasCloseBox || inAttributes.fHasZoomBox)
				outWindowStyle |= WS_SYSMENU;
			if (inAttributes.fHasZoomBox)
				outWindowStyle |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
			if (inAttributes.fResizable)
				outWindowStyle |= WS_THICKFRAME;
			break;
			}
		case ZOSWindow::lookPalette:
			{
			outWindowStyle |= WS_BORDER | WS_DLGFRAME;
			if (inAttributes.fHasCloseBox || inAttributes.fHasZoomBox)
				outWindowStyle |= WS_SYSMENU;
			if (inAttributes.fHasZoomBox)
				outWindowStyle |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
			if (inAttributes.fResizable)
				outWindowStyle |= WS_THICKFRAME;

			outExWindowStyle |= WS_EX_TOOLWINDOW;
			break;
			}
		case ZOSWindow::lookAlert:
		case ZOSWindow::lookMovableAlert:
			{
			outWindowStyle |= WS_POPUP | WS_BORDER;
			if (inAttributes.fResizable)
				outWindowStyle |= WS_THICKFRAME;
			break;
			}
		case ZOSWindow::lookModal:
		case ZOSWindow::lookMovableModal:
			{
			outExWindowStyle = WS_EX_DLGMODALFRAME;
			outWindowStyle |= WS_POPUP | WS_BORDER | WS_DLGFRAME;
			if (inAttributes.fResizable)
				outWindowStyle |= WS_THICKFRAME;
			break;
			}
		case ZOSWindow::lookThinBorder:
		case ZOSWindow::lookMenu:
			{
			outWindowStyle |= WS_POPUP | WS_BORDER;
			break;
			}
		default:
			ZUnimplemented();
		};

// AG 99-04-13. *DONT* use topmost style -- it's not at all the same
// as a floater on the Mac, and it plays havoc with our layer management.
//	if (inLayer == ZOSWindow::layerFloater)
//		outExWindowStyle |= WS_EX_TOPMOST;
	}

static HWND sGetTopmostVisibleModal()
	{
	HWND modalHWND = nil;
	if (ZUtil_Win::sUseWAPI())
		{
		for (HWND iterHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
			iterHWND != nil && modalHWND == nil; iterHWND = ::GetWindow(iterHWND, GW_HWNDNEXT))
			{
			if (::IsWindowVisible(iterHWND))
				{
				if (ZOSWindow_TopLevel* iterWindow
					= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayW(iterHWND)))
					{
					if (iterWindow->Internal_IsModal())
						modalHWND = iterHWND;
					}
				}
			}
		}
	else
		{
		for (HWND iterHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
			iterHWND != nil && modalHWND == nil; iterHWND = ::GetWindow(iterHWND, GW_HWNDNEXT))
			{
			if (::IsWindowVisible(iterHWND))
				{
				if (ZOSWindow_TopLevel* iterWindow
					= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayA(iterHWND)))
					{
					if (iterWindow->Internal_IsModal())
						modalHWND = iterHWND;
					}
				}
			}
		}
	return modalHWND;
	}

static void sStuffMouseState(HWND inHWND, bool inCaptured, ZOSWindow_Win::MouseState& outState)
	{
	DWORD cursorPos = ::GetMessagePos();
	POINT currentPOINT;
	currentPOINT.x = short(LOWORD(cursorPos));
	currentPOINT.y = short(HIWORD(cursorPos));
	::ScreenToClient(inHWND, &currentPOINT);

	outState.fLocation.h = currentPOINT.x;
	outState.fLocation.v = currentPOINT.y;

	RECT clientRECT;
	::GetClientRect(inHWND, &clientRECT);
	ZRect clientRect(clientRECT);
	outState.fContained = clientRect.Contains(outState.fLocation);

	outState.fCaptured = inCaptured;

	bool leftIsDown = ::GetKeyState(VK_LBUTTON) & 0x8000;
	bool rightIsDown = ::GetKeyState(VK_RBUTTON) & 0x8000;
	if (::GetSystemMetrics(SM_SWAPBUTTON))
		swap(leftIsDown, rightIsDown);
	outState.fButtons[0] = leftIsDown;
	outState.fButtons[1] = (::GetKeyState(VK_MBUTTON) & 0x8000);
	outState.fButtons[2] = rightIsDown;
	outState.fButtons[3] = false;
	outState.fButtons[4] = false;

	outState.fModifier_Command = false;
	outState.fModifier_Option = false;
	outState.fModifier_Shift = ::GetKeyState(VK_SHIFT) & 0x8000;
	outState.fModifier_Control = ::GetKeyState(VK_CONTROL) & 0x8000;
	outState.fModifier_CapsLock = ::GetKeyState(VK_CAPITAL) & 0x8000;
	}

LRESULT ZOSWindow_Win::WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_Destroy)
		{
		::DestroyWindow(fHWND);
		return 0;
		}
	else if (inMessage == sMSG_AcquireCapture)
		{
		fRequestedCapture = true;
		::SetCapture(fHWND);
		return 0;
		}
	else if (inMessage == sMSG_ReleaseCapture)
		{
		if (fRequestedCapture)
			{
			fRequestedCapture = false;
			::ReleaseCapture();
			}
		return 0;
		}
	else if (inMessage == sMSG_Menu)
		{
		return this->CallBaseWindowProc(fHWND, WM_SYSCOMMAND, inWPARAM, inLPARAM);
		}
	else if (inMessage == sMSG_Show)
		{
		if (::IsWindowVisible(fHWND))
			{
			if (inWPARAM == eZShownStateHidden)
				{
				::MySetWindowPos(fHWND, nil, 0, 0, 0, 0,
					SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
				}
			}
		else
			{
			if (inWPARAM == eZShownStateShown)
				{
				::MySetWindowPos(fHWND, nil, 0, 0, 0, 0,
					SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
				}
			}
		return 0;
		}

	switch (inMessage)
		{
		case WM_MOUSEACTIVATE:
			{
			// We handle activation and stacking when the mouse down is delivered
			return MA_NOACTIVATE;
			}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
			HWND topmostModal = ::sGetTopmostVisibleModal();
			if (topmostModal != nil && topmostModal != fHWND)
				{
				::sSendMessage(fHWND, sMSG_BringFront, 0, 0);
				::MessageBeep(0);
				return 0;
				}
			break;
			}
		case WM_NCLBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
			{
			HWND topmostModal = ::sGetTopmostVisibleModal();
			if (fHWND == topmostModal)
				topmostModal = nil;
			switch (inWPARAM)
				{
				case HTCLIENT:
				case HTCLOSE:
				case HTHELP:
				case HTMENU:
					{
					::sSendMessage(fHWND, sMSG_BringFront, 0, 0);
					if (topmostModal)
						{
						::MessageBeep(0);
						return 0;
						}
					break;
					}
				case HTCAPTION:
				case HTBORDER:
				case HTBOTTOM:
				case HTBOTTOMLEFT:
				case HTBOTTOMRIGHT:
				case HTGROWBOX:
				case HTLEFT:
				case HTRIGHT:
				case HTTOP:
				case HTTOPLEFT:
				case HTTOPRIGHT:
					{
					// If the control key is not down then bring front
					if (!(::GetKeyState(VK_CONTROL) & 0x8000))
						::sSendMessage(fHWND, sMSG_BringFront, 0, 0);
					}
				}
			if (inMessage == WM_NCLBUTTONDOWN)
				{
				switch (inWPARAM)
					{
					case HTMINBUTTON:
					case HTMAXBUTTON:
					case HTCLIENT:
					case HTCLOSE:
					case HTSYSMENU:
					case HTMENU:
					case HTHELP:
						{
						// Default behavior for the buttons etc.
						return this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
						}
					case HTCAPTION:
						{
						bool priorDisallowActivationChange = sDisallowActivationChange;
						sDisallowActivationChange = true;
						::sSendMessage(fHWND, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, inLPARAM);
						sDisallowActivationChange = priorDisallowActivationChange;
						return 0;
						}
					case HTBOTTOM:
					case HTBOTTOMLEFT:
					case HTBOTTOMRIGHT:
					case HTGROWBOX:
					case HTLEFT:
					case HTRIGHT:
					case HTTOP:
					case HTTOPLEFT:
					case HTTOPRIGHT:
						{
						bool priorDisallowActivationChange = sDisallowActivationChange;
						sDisallowActivationChange = true;
						::sSendMessage(fHWND, WM_SYSCOMMAND, SC_SIZE + inWPARAM - 9, inLPARAM);
						sDisallowActivationChange = priorDisallowActivationChange;
						return 0;
						}
					}
				}
			break;
			}
		}

	switch (inMessage)
		{
		case WM_CREATE:
			{
			fDropTarget = new DropTarget(this);
			::RegisterDragDrop(fHWND, fDropTarget);
			break;
			}
		case WM_DESTROY:
			{
			::RevokeDragDrop(fHWND);
			fDropTarget->Release();
			fDropTarget = nil;
			break;
			}
		case WM_NCDESTROY:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			fHWND = nil;
			return result;
			}
		case WM_SETFOCUS:
			{
			break;
			}
		case WM_PALETTECHANGED:
			{
			if (reinterpret_cast<HWND>(inWPARAM) == fHWND)
				return 1;
			}
			// fall through
		case WM_QUERYNEWPALETTE:
			{
			ZMutexLocker structureLocker(fMutex_Structure);
			if (fCanvas)
				{
				::UpdateColors(fCanvas->GetHDC());
				}
			else
				{
				HDC theHDC = ::GetDC(fHWND);
				HPALETTE oldHPALETTE = ::SelectPalette(theHDC, fHPALETTE, false);
				int countChanged = ::RealizePalette(theHDC);
				::UpdateColors(theHDC);
				::ReleaseDC(fHWND, theHDC);
				}
			::InvalidateRect(fHWND, (LPRECT)nil, false);
			return 1;
			}
		case WM_CLOSE:
			{
			this->Internal_RecordCloseBoxHit();
			return 0;
			}
		case WM_SETCURSOR:
			{
			if (inHWND == fHWND && LOWORD(inLPARAM) == HTCLIENT)
				{
				if (this->IsBusy())
					::sSetCursor(ZCursor(ZCursor::eCursorTypeWatch));
				else
					::sSetCursor(fCursor);
				return 0;
				}
			return this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			}
		case WM_CAPTURECHANGED:
			{
			if (fRequestedCapture && fHWND != reinterpret_cast<HWND>(inLPARAM))
				{
				fRequestedCapture = false;
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:CaptureCancelled");
				this->QueueMessageForOwner(theMessage);
				}
			break;
			}
		#ifdef WM_MOUSEWHEEL
		case WM_MOUSEWHEEL:
			{
			ZDebugLogf(kDebug_Win, ("WM_MOUSEWHEEL"));
			return 0;
			}
		#endif // WM_MOUSEWHEEL
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			{
			fCheckMouseOnTimer = true;

			MouseState theState;
			theState.fLocation.h = short(LOWORD(inLPARAM));
			theState.fLocation.v = short(HIWORD(inLPARAM));

			RECT clientRECT;
			::GetClientRect(fHWND, &clientRECT);
			ZRect clientRect(clientRECT);
			theState.fContained = clientRect.Contains(theState.fLocation);

			fContainedMouse = theState.fContained;

			theState.fCaptured = fRequestedCapture;

			theState.fButtons[0] = ((inWPARAM & MK_LBUTTON) != 0);
			theState.fButtons[1] = ((inWPARAM & MK_MBUTTON) != 0);
			theState.fButtons[2] = ((inWPARAM & MK_RBUTTON) != 0);
			theState.fButtons[3] = false;
			theState.fButtons[4] = false;

			theState.fModifier_Command = false;
			theState.fModifier_Option = false;
			theState.fModifier_Shift = ((inWPARAM & MK_SHIFT) != 0);
			theState.fModifier_Control = ((inWPARAM & MK_CONTROL) != 0);
			theState.fModifier_CapsLock = ::GetKeyState(VK_CAPITAL) & 0x8000;

			int buttonDownIndex = -1;
			if ((inMessage == WM_LBUTTONDOWN) && (0 == (inWPARAM & (MK_MBUTTON | MK_RBUTTON))))
				buttonDownIndex = 0;
			else if ((inMessage == WM_MBUTTONDOWN) && (0 == (inWPARAM & (MK_LBUTTON | MK_RBUTTON))))
				buttonDownIndex = 1;
			else if ((inMessage == WM_RBUTTONDOWN) && (0 == (inWPARAM & (MK_LBUTTON | MK_MBUTTON))))
				buttonDownIndex = 2;

			bigtime_t theMessageTime = bigtime_t(::GetMessageTime()) * 1000;
			if (buttonDownIndex != -1)
				{
				this->Internal_RecordMouseDown(buttonDownIndex, theState, theMessageTime);
				}
			else if ((inMessage == WM_LBUTTONUP
				|| inMessage == WM_MBUTTONUP
				|| inMessage == WM_RBUTTONUP)
				&& (0 == (inWPARAM & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))))
				{
				this->Internal_RecordMouseUp(theState, theMessageTime);
				}
			else
				{
				this->Internal_RecordMouseState(theState, theMessageTime);
				}
			return 0;
			}
		case WM_TIMER:
			{
			this->Internal_RecordIdle();

			if (fCheckMouseOnTimer || fRequestedCapture)
				{
				MouseState theState;
				::sStuffMouseState(fHWND, fRequestedCapture, theState);

				if (!theState.fContained)
					fCheckMouseOnTimer = false;

				fContainedMouse = theState.fContained;

				bigtime_t theMessageTime = bigtime_t(::GetMessageTime()) * 1000;
				this->Internal_RecordMouseState(theState, theMessageTime);
				}
			return 0;
			}
		case WM_NCLBUTTONDOWN:
			{
			if (inWPARAM == HTSYSMENU)
				{
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:TitleIcon");
				ZPoint theLocation(short(LOWORD(inLPARAM)), short(HIWORD(inLPARAM)));
				theMessage.SetPoint("where", theLocation);
				theMessage.SetInt32("WPARAM", inWPARAM);
				theMessage.SetInt32("LPARAM", inLPARAM);
				this->QueueMessageForOwner(theMessage);
				return 0;
				}
			break;
			}
		case WM_NCHITTEST:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			if (!fHasResizeBorder)
				{
				switch (result)
					{
					case HTBORDER:
					case HTBOTTOM:
					case HTBOTTOMLEFT:
					case HTBOTTOMRIGHT:
					case HTGROWBOX:
					case HTLEFT:
					case HTRIGHT:
					case HTTOP:
					case HTTOPLEFT:
					case HTTOPRIGHT:
						result = HTCAPTION;
						break;
					}
				}

			if (fHasSizeBox)
				{
				POINT clientPOINT;
				clientPOINT.x = short(LOWORD(inLPARAM));
				clientPOINT.y = short(HIWORD(inLPARAM));
				::ScreenToClient(inHWND, &clientPOINT);
				if (fSizeBoxFrame.Contains(clientPOINT))
					result = HTBOTTOMRIGHT;
				}

			if (!fHasCloseBox && result == HTCLOSE)
				result = HTCAPTION;

			return result;
			}
		case WM_ACTIVATE:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			this->Internal_RecordActiveChange(LOWORD(inWPARAM) != WA_INACTIVE);
			return result;
			}
		case WM_ACTIVATEAPP:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			this->Internal_RecordAppActiveChange(LOWORD(inWPARAM));
			return result;
			}
// WM_KEYDOWN, the raw key
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			{
			if (/*inMessage == WM_SYSKEYDOWN || */inMessage == WM_KEYDOWN)
				{
				if (fHasMenuBar
					&& (::GetKeyState(VK_CONTROL) & 0x8000) && !(::GetKeyState(VK_MENU) & 0x8000))
					{
					if (!fRequestedCapture && (inWPARAM != VK_CONTROL))
						{
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:WinMenuKey");
						theMessage.SetInt32("virtualKey", inWPARAM);
						theMessage.SetBool("shift", ::GetKeyState(VK_SHIFT) & 0x8000);
						theMessage.SetBool("control", true);
						theMessage.SetBool("alt", ::GetKeyState(VK_MENU) & 0x8000);
						this->QueueMessage(theMessage);
						}
					return 0;
					}
				}

			if (inWPARAM == VK_SHIFT || inWPARAM == VK_CONTROL || inWPARAM == VK_CAPITAL)
				{
				MouseState theState;
				::sStuffMouseState(fHWND, fRequestedCapture, theState);

				if (!theState.fContained)
					fCheckMouseOnTimer = false;

				bigtime_t theMessageTime = bigtime_t(::GetMessageTime()) * 1000;
				this->Internal_RecordMouseState(theState, theMessageTime);
				}

			if (/*inMessage == WM_SYSKEYDOWN || */inMessage == WM_KEYDOWN)
				{
				BYTE keyboardState[256];
				::GetKeyboardState(keyboardState);

				ZKeyboard::CharCode ourCharCode = ZKeyboard::ccUnknown;

				WORD translatedChar;
				int result = ::ToAscii(inWPARAM, HIWORD(inLPARAM),
					keyboardState, &translatedChar, 0);
				if (result == 0)
					{
					switch (inWPARAM)
						{
						case VK_CLEAR: ourCharCode = ZKeyboard::ccClear; break;
						case VK_ESCAPE: ourCharCode = ZKeyboard::ccEscape; break;
						case VK_PRIOR: ourCharCode = ZKeyboard::ccPageUp; break;
						case VK_NEXT: ourCharCode = ZKeyboard::ccPageDown; break;
						case VK_END: ourCharCode = ZKeyboard::ccEnd; break;
						case VK_HOME: ourCharCode = ZKeyboard::ccHome; break;
						case VK_LEFT: ourCharCode = ZKeyboard::ccLeft; break;
						case VK_UP: ourCharCode = ZKeyboard::ccUp; break;
						case VK_RIGHT: ourCharCode = ZKeyboard::ccRight; break;
						case VK_DOWN: ourCharCode = ZKeyboard::ccDown; break;
						case VK_DELETE: ourCharCode = ZKeyboard::ccFwdDelete; break;
						case VK_HELP: ourCharCode = ZKeyboard::ccHelp; break;
						}
					}
				else if (result == 1)
					{
					ourCharCode = translatedChar;
					}
				if (ourCharCode != ZKeyboard::ccUnknown)
					{
					if (!fRequestedCapture)
						{
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:KeyDown");
						theMessage.SetInt64("when", bigtime_t(::GetMessageTime()) * 1000);
						theMessage.SetInt32("charCode", ourCharCode);
						theMessage.SetBool("shift", ::GetKeyState(VK_SHIFT) & 0x8000);
						theMessage.SetBool("control", ::GetKeyState(VK_CONTROL) & 0x8000);
						theMessage.SetBool("capsLock", ::GetKeyState(VK_CAPITAL) & 0x8000);
						this->QueueMessageForOwner(theMessage);
						}
					return 0;
					}
				}
			break;
			}
		case WM_CHAR:
			ZDebugLogf(0, ("Got a WM_CHAR somehow"));
			break;
		case WM_GETMINMAXINFO:
			{
			this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);

			// Note that Windows imposes additional minimum size constraints on windows based on
			// their attributes (sysmenu, minmize/maximize/close boxes etc). So the minium size we
			// set may well be overridden.
			ZPoint theTopLeftInset, theBottomRightInset;
			sCalcContentInsets(fHWND, theTopLeftInset, theBottomRightInset);

			MINMAXINFO* theMMI = reinterpret_cast<MINMAXINFO*>(inLPARAM);
			theMMI->ptMinTrackSize.x = fSize_Min.h + theTopLeftInset.h - theBottomRightInset.h;
			theMMI->ptMinTrackSize.y = fSize_Min.v + theTopLeftInset.v - theBottomRightInset.v;
			theMMI->ptMaxTrackSize.x = fSize_Max.h + theTopLeftInset.h - theBottomRightInset.h;
			theMMI->ptMaxTrackSize.y = fSize_Max.v + theTopLeftInset.v - theBottomRightInset.v;
			return 0;
			}
		case WM_ERASEBKGND:
			{
			if (fOwner)
				return 1;
			break;
			}
		case WM_PAINT:
			{
			/* AG 2000-07-27.
			There is a serious problem with Windows' BeginPaint/EndPaint model. BeginPaint
			has several jobs. First it clears the pending update flag in the internal
			window structure, without which we'll have WM_PAINT messages posted to us
			continuously, even if we manually clear the update region. Second it validates (clears)
			the windows' update region. Third, it returns a DC which has some kind of
			internal clipping set to the update region (before it was cleared of course).
			The weakness is that the PAINTSTRUCT filled out by BeginPaint does not have any
			indication of the region we're supposed to paint, just the bounds of that
			region. I have searched high and low through all my docs and the web and have
			not been able to identify any means of determining what that region is. Our
			update management model relies on getting a good value for updates that come
			via WM_PAINT (generally due to windows being dragged over another window). We
			call GetUpdateRgn just before we call BeginPaint -- that gives us the update
			region before it gets validated. If some additional pixels got invalidated
			between the two calls, then we have no way of finding out about them. To
			ameliorate the effect of this I check the PAINTSTRUCT's rcPaint member, and if
			it does not match the bounds of the update region I add the whole of that rect
			into the update region. This will obviously invalidate more pixels than it
			should at some times, and will miss some pixels at others. It only comes into
			play when there's a race between the calls to GetUpdateRgn and BeginPaint,
			which doesn't happen often.
			*/
			ZDCRgn theUpdateRgn;
			::GetUpdateRgn(fHWND, theUpdateRgn.GetHRGN(), false);
			PAINTSTRUCT thePS;
			HDC paintHDC = ::BeginPaint(fHWND, &thePS);
			// If thePS.rcPaint describes a different boundary than what we got from GetUpdateRgn,
			// add the entire rect into theUpdateRgn. See long-winded explanation above.
			ZRect updateBounds = theUpdateRgn.Bounds();
			if (updateBounds != thePS.rcPaint)
				theUpdateRgn |= thePS.rcPaint;

			if (fOwner)
				{
				bool isActive = this->Internal_GetReportedActive();
				ZDCInk backInk = fOwner->OwnerGetBackInk(isActive);

				ZRef<ZDCCanvas_GDI> nativeCanvas
					= ZDCCanvas_GDI::sFindCanvasOrCreateNative(paintHDC);

				ZDC theDC(nativeCanvas, ZPoint::sZero);

				theDC.SetClip(theUpdateRgn);
				theDC.SetInk(backInk);
				theDC.Fill(theDC.GetClip());
				if (fHasSizeBox && (theUpdateRgn & fSizeBoxFrame))
					{
					ZRGBColor colorFace = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_3DFACE));
					ZRGBColor colorHilight
						= ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_3DHILIGHT));
					ZRGBColor colorShadow = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_3DSHADOW));

					ZRect r = fSizeBoxFrame;
					theDC.SetInk(colorFace);
					theDC.Fill(r);

					theDC.Pixel(r.left, r.bottom-1, colorHilight);
					theDC.Pixel(r.right-1, r.top, colorHilight);

					theDC.SetInk(colorHilight);
					theDC.Line(r.left, r.bottom-2, r.right-2, r.top);
					theDC.Line(r.left+4, r.bottom-2, r.right-2, r.top+4);
					theDC.Line(r.left+8, r.bottom-2, r.right-2, r.top+8);

					theDC.SetInk(colorShadow);
					theDC.Line(r.left+1, r.bottom-2, r.right-2, r.top+1);
					theDC.Line(r.left+2, r.bottom-2, r.right-2, r.top+2);
					theDC.Line(r.left+5, r.bottom-2, r.right-2, r.top+5);
					theDC.Line(r.left+6, r.bottom-2, r.right-2, r.top+6);
					theDC.Line(r.left+9, r.bottom-2, r.right-2, r.top+9);
					theDC.Line(r.left+10, r.bottom-2, r.right-2, r.top+10);
					}
				}
			::EndPaint(fHWND, &thePS);

			ZMutexLocker locker(fMutex_Structure);
			if (fCanvas)
				fCanvas->InvalClip();
			this->Internal_RecordInvalidation(theUpdateRgn);
			return 0;
			}
		case WM_WINDOWPOSCHANGED:
			{
			// Must call the base window proc, otherwise MDI
			// minimize/maximize does not munge the menu bar.
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			WINDOWPOS* theWINDOWPOS = reinterpret_cast<WINDOWPOS*>(inLPARAM);
			if ((theWINDOWPOS->flags & SWP_FRAMECHANGED)
				|| ((theWINDOWPOS->flags & (SWP_NOSIZE | SWP_NOMOVE)) != (SWP_NOSIZE | SWP_NOMOVE)))
				{
				ZPoint theTopLeftInset, theBottomRightInset;
				sCalcContentInsets(fHWND, theTopLeftInset, theBottomRightInset);
				if ((theWINDOWPOS->flags & SWP_FRAMECHANGED)
					|| ((theWINDOWPOS->flags & (SWP_NOSIZE)) == 0))
					{
					if (fHasSizeBox)
						{
						// Update our notion of where our size box is located. Has to be done under
						// the protection of our structure mutex so that our canvas will not
						// be drawing at the same time.
						ZMutexLocker locker(fMutex_Structure);
						ZRect oldSizeBoxFrame = fSizeBoxFrame;
						fSizeBoxFrame.right =
							theWINDOWPOS->cx - theTopLeftInset.h + theBottomRightInset.h;
						fSizeBoxFrame.bottom =
							theWINDOWPOS->cy - theTopLeftInset.v + theBottomRightInset.v;
						fSizeBoxFrame.left = fSizeBoxFrame.right - 13;
						fSizeBoxFrame.top = fSizeBoxFrame.bottom - 13;
						if (fSizeBoxFrame != oldSizeBoxFrame)
							{
							if (fCanvas)
								fCanvas->InvalClip();
							ZDCRgn invalidRgn = oldSizeBoxFrame;
							invalidRgn |= fSizeBoxFrame;
							::InvalidateRgn(fHWND, invalidRgn.GetHRGN(), true);
							}
						}
					}
				if ((theWINDOWPOS->flags & SWP_FRAMECHANGED)
					|| ((theWINDOWPOS->flags & (SWP_NOSIZE | SWP_NOMOVE)) == 0))
					{
					// Both size and position changed.
					ZRect theNewFrame;
					theNewFrame.left = theWINDOWPOS->x + theTopLeftInset.h;
					theNewFrame.top = theWINDOWPOS->y + theTopLeftInset.v;
					theNewFrame.right = theWINDOWPOS->x + theWINDOWPOS->cx + theBottomRightInset.h;
					theNewFrame.bottom = theWINDOWPOS->y + theWINDOWPOS->cy + theBottomRightInset.v;
					this->Internal_RecordFrameChange(theNewFrame);
					}
				else if ((theWINDOWPOS->flags & SWP_FRAMECHANGED)
					|| ((theWINDOWPOS->flags & SWP_NOSIZE) == 0))
					{
					// Just the size changed.
					this->Internal_RecordSizeChange(
						ZPoint(theWINDOWPOS->cx - theTopLeftInset.h + theBottomRightInset.h,
						theWINDOWPOS->cy - theTopLeftInset.v + theBottomRightInset.v));
					}
				else if ((theWINDOWPOS->flags & SWP_FRAMECHANGED)
					|| ((theWINDOWPOS->flags & SWP_NOMOVE) == 0))
					{
					// Just the position changed.
					this->Internal_RecordLocationChange(
						ZPoint(theWINDOWPOS->x + theTopLeftInset.h,
						theWINDOWPOS->y + theTopLeftInset.v));
					}
				}
			if (theWINDOWPOS->flags & SWP_SHOWWINDOW)
				{
				this->Internal_RecordShownStateChange(eZShownStateShown);
				ZOSApp_Win::sGet()->Internal_RecordWindowRosterChange();
				}
			else if (theWINDOWPOS->flags & SWP_HIDEWINDOW)
				{
				this->Internal_RecordShownStateChange(eZShownStateHidden);
				ZOSApp_Win::sGet()->Internal_RecordWindowRosterChange();
				}
			return result;
			}
// ----------
// Menus
		case WM_MENUCHAR:
			{
			return ZMenu_Win::sHandle_MENUCHAR(LOWORD(inWPARAM), HIWORD(inWPARAM),
				reinterpret_cast<HMENU>(inLPARAM));
			}
		case WM_MEASUREITEM:
			{
			MEASUREITEMSTRUCT* theMIS = reinterpret_cast<MEASUREITEMSTRUCT*>(inLPARAM);
			if (theMIS->CtlType == ODT_MENU)
				{
				ZMenu_Win::sHandle_MEASUREITEM(theMIS);
				return 1;
				}
			break;
			}
		case WM_DRAWITEM:
			{
			DRAWITEMSTRUCT* theDIS = reinterpret_cast<DRAWITEMSTRUCT*>(inLPARAM);
			if (theDIS->CtlType == ODT_MENU)
				{
				ZMenu_Win::sHandle_DRAWITEM(theDIS);
				return 1;
				}
			break;
			}
// ----------
// Miscellaneous
		case WM_DISPLAYCHANGE:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:ScreenLayoutChanged");
			this->QueueMessageForOwner(theMessage);
			break;
			}
		}

	return this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_Win::sWindowProcW(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Win* theWindow = sFromHWNDNilOkayW(inHWND);
	if (!theWindow)
		{
		if (inMessage == WM_GETMINMAXINFO)
			{
			// The very first message sent to a window is (go figure)
			// WM_GETMINMAXINFO, so we have to handle it specially.
			return ::DefWindowProcW(inHWND, inMessage, inWPARAM, inLPARAM);
			}
		else if (inMessage == WM_NCCREATE)
			{
			// If this is the first message sent to a window, attach the ZOSWindow to it
			CREATESTRUCTW* theCREATESTRUCT = reinterpret_cast<CREATESTRUCTW*>(inLPARAM);
			theWindow = reinterpret_cast<ZOSWindow_Win*>(theCREATESTRUCT->lpCreateParams);
			theWindow->fHWND = inHWND;
			// Attach the marker property
			::SetPropW(inHWND, L"ZOSWindow_Win", reinterpret_cast<HANDLE>(theWindow));
			// And set the user data
			::SetWindowLongW(inHWND, GWL_USERDATA, reinterpret_cast<LONG>(theWindow));
			}
		}

	ZAssertStop(kDebug_Win, theWindow != nil);
	if (theWindow->fWindowProcFilter_Head)
		return theWindow->fWindowProcFilter_Head->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	return theWindow->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_Win::sWindowProcA(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Win* theWindow = sFromHWNDNilOkayA(inHWND);
	if (!theWindow)
		{
		if (inMessage == WM_GETMINMAXINFO)
			{
			// The very first message sent to a window is (go figure)
			// WM_GETMINMAXINFO, so we have to handle it specially.
			return ::DefWindowProcA(inHWND, inMessage, inWPARAM, inLPARAM);
			}
		else if (inMessage == WM_NCCREATE)
			{
			// If this is the first message sent to a window, attach the ZOSWindow to it
			CREATESTRUCTA* theCREATESTRUCT = reinterpret_cast<CREATESTRUCTA*>(inLPARAM);
			theWindow = reinterpret_cast<ZOSWindow_Win*>(theCREATESTRUCT->lpCreateParams);
			theWindow->fHWND = inHWND;
			// Attach the marker property
			::SetPropA(inHWND, "ZOSWindow_Win", reinterpret_cast<HANDLE>(theWindow));
			// And set the user data
			::SetWindowLongA(inHWND, GWL_USERDATA, reinterpret_cast<LONG>(theWindow));
			}
		}

	ZAssertStop(kDebug_Win, theWindow != nil);
	if (theWindow->fWindowProcFilter_Head)
		return theWindow->fWindowProcFilter_Head->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	return theWindow->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

// Once again, windows can be a real pain in the butt. We have to have a separate
// window proc for MDI children just to handle the create case. The lpCreateParams
// points not to our create struct, but instead contains a pointer to a MDICREATESTRUCT,
// whose lParam contains a pointer to our CreateStruct
LRESULT ZOSWindow_Win::sWindowProc_MDIW(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Win* theWindow = sFromHWNDNilOkayW(inHWND);
	if (!theWindow)
		{
		if (inMessage == WM_GETMINMAXINFO)
			{
			// The very first message sent to a window is (go figure)
			// WM_GETMINMAXINFO, so we have to handle it specially.
			return ::DefMDIChildProcW(inHWND, inMessage, inWPARAM, inLPARAM);
			}
		else if (inMessage == WM_NCCREATE)
			{
			// If this is the first message sent to a window, attach the ZOSWindow to it
			CREATESTRUCTW* theCREATESTRUCT = reinterpret_cast<CREATESTRUCTW*>(inLPARAM);
			MDICREATESTRUCTW* theMDICREATESTRUCT
				= reinterpret_cast<MDICREATESTRUCTW*>(theCREATESTRUCT->lpCreateParams);
			theWindow = reinterpret_cast<ZOSWindow_MDI*>(theMDICREATESTRUCT->lParam);
			theWindow->fHWND = inHWND;
			// Attach the marker property
			::SetPropW(inHWND, L"ZOSWindow_Win", reinterpret_cast<HANDLE>(theWindow));
			// And set the user data
			::SetWindowLongW(inHWND, GWL_USERDATA, reinterpret_cast<LONG>(theWindow));
			}
		}

	ZAssertStop(kDebug_Win, theWindow != nil);
	if (theWindow->fWindowProcFilter_Head)
		return theWindow->fWindowProcFilter_Head->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	return theWindow->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_Win::sWindowProc_MDIA(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Win* theWindow = sFromHWNDNilOkayA(inHWND);
	if (!theWindow)
		{
		if (inMessage == WM_GETMINMAXINFO)
			{
			// The very first message sent to a window is (go figure)
			// WM_GETMINMAXINFO, so we have to handle it specially.
			return ::DefMDIChildProcA(inHWND, inMessage, inWPARAM, inLPARAM);
			}
		else if (inMessage == WM_NCCREATE)
			{
			// If this is the first message sent to a window, attach the ZOSWindow to it
			CREATESTRUCTA* theCREATESTRUCT = reinterpret_cast<CREATESTRUCTA*>(inLPARAM);
			MDICREATESTRUCTA* theMDICREATESTRUCT =
				reinterpret_cast<MDICREATESTRUCTA*>(theCREATESTRUCT->lpCreateParams);
			theWindow = reinterpret_cast<ZOSWindow_MDI*>(theMDICREATESTRUCT->lParam);
			theWindow->fHWND = inHWND;
			// Attach the marker property
			::SetPropA(inHWND, "ZOSWindow_Win", reinterpret_cast<HANDLE>(theWindow));
			// And set the user data
			::SetWindowLongA(inHWND, GWL_USERDATA, reinterpret_cast<LONG>(theWindow));
			}
		}

	ZAssertStop(kDebug_Win, theWindow != nil);
	if (theWindow->fWindowProcFilter_Head)
		return theWindow->fWindowProcFilter_Head->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	return theWindow->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win::WindowProcFilter

ZOSWindow_Win::WindowProcFilter::WindowProcFilter(ZOSWindow_Win* inWindow)
	{
	ZAssertStop(kDebug_Win, inWindow);
	fWindow = inWindow;
	fPrev = nil;
	fNext = fWindow->fWindowProcFilter_Head;
	if (fNext)
		fNext->fPrev = this;
	fWindow->fWindowProcFilter_Head = this;
	}

ZOSWindow_Win::WindowProcFilter::~WindowProcFilter()
	{
	if (fPrev)
		fPrev->fNext = fNext;
	if (fNext)
		fNext->fPrev = fPrev;
	if (fWindow->fWindowProcFilter_Head == this)
		fWindow->fWindowProcFilter_Head = fNext;
	}

LRESULT ZOSWindow_Win::WindowProcFilter::WindowProc(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	LRESULT result;
	if (fNext)
		result = fNext->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	else
		result = fWindow->WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	if (inMessage == WM_NCDESTROY)
		delete this;
	return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_TopLevel

static int32 sLayerToZLevel(ZOSWindow::Layer inLayer)
	{
	switch (inLayer)
		{
		case ZOSWindow::layerSinker: return 0;
		case ZOSWindow::layerDocument: return 1;
		case ZOSWindow::layerFloater: return 2;
		case ZOSWindow::layerDialog: return 3;
		case ZOSWindow::layerMenu: return 4;
		case ZOSWindow::layerHelp: return 5;
		}
	ZUnimplemented();
	return 0;
	}

ZOSWindow_TopLevel::ZOSWindow_TopLevel(bool inHasSizeBox, bool inHasResizeBorder,
	bool inHasCloseBox, bool inHasMenuBar, ZOSWindow::Layer inLayer)
:	ZOSWindow_Win(inHasSizeBox, inHasResizeBorder,
	inHasCloseBox, inHasMenuBar, sLayerToZLevel(inLayer))
	{
	fHWND_TaskBarDummy = nil;

	// Here we break the layer param into its component behaviors -- what z level does
	// it have, should it be hidden when the app is suspended (some other app is
	// activated), and is it modal?
	fCanMoveWithinLevel = true;
	fCanBeTargetted = true;
	fIsModal = false;
	fAppearsOnTaskBar = true;

	fHideOnSuspend = false;
	fVisibleBeforeSuspend = false;

	fSuspended = false;

	switch (inLayer)
		{
		case layerFloater:
			fCanBeTargetted = false;
			fHideOnSuspend = true;
			fAppearsOnTaskBar = false;
			break;
		case layerDialog:
			fCanMoveWithinLevel = false;
			fIsModal = true;
			fAppearsOnTaskBar = false;
			break;
		case layerMenu:
			fCanBeTargetted = false;
			fAppearsOnTaskBar = false;
			break;
		case layerHelp:
			fCanBeTargetted = false;
			fAppearsOnTaskBar = false;
			break;
		}

	ZOSApp_Win::sGet()->AddOSWindow(this);
	}

ZOSWindow_TopLevel::~ZOSWindow_TopLevel()
	{
	ZAssertStop(kDebug_Win, fHWND_TaskBarDummy == nil);

	ZOSApp_Win::sGet()->RemoveOSWindow(this);
	}

static HWND sHandleWindowPosChangingW(ZOSWindow_TopLevel* inWindow, HWND inRequestedAfter)
	{
	// Two cases, either the window is being moved higher up the stack, or lower.

	// First scan from top to bottom till we hit our window. If we don't hit the inRequestedAfter
	// window before hitting our window then don't do anything -- that implies our window is being
	// moved lower in the stack
	bool hitRequestedAfter = false;
	if (inRequestedAfter == HWND_TOPMOST)
		hitRequestedAfter = true;
	else if (inRequestedAfter == HWND_BOTTOM)
		inRequestedAfter = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);

	HWND realHWNDAfter = inRequestedAfter;
	HWND currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDFIRST);
	while (currentHWND && currentHWND != inWindow->GetHWND())
		{
		HWND nextHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		if (inRequestedAfter == HWND_TOP
			&& ((::GetWindowLongW(currentHWND, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0))
			{
			hitRequestedAfter = true;
			}

		if (currentHWND == inRequestedAfter)
			hitRequestedAfter = true;

		if (hitRequestedAfter)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				LONG currentWindowStyle = ::GetWindowLongW(currentHWND, GWL_STYLE);
				if (!(currentWindowStyle & WS_MINIMIZE))
					{
					if (currentTopLevel->Internal_GetZLevel() > inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = currentHWND;
						}
					else if (!currentTopLevel->Internal_CanMoveWithinLevel()
						&& currentTopLevel->Internal_GetZLevel() == inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = currentHWND;
						}
					}
				}
			}
		currentHWND = nextHWND;
		}

	if (!hitRequestedAfter)
		{
		// We must be moving the window down the stack, so start at the bottom
		// and move each window in a lower layer so it is behind inRequestedAfter
		currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);
		while (currentHWND && currentHWND != inWindow->GetHWND())
			{
			HWND previousHWND = ::GetWindow(currentHWND, GW_HWNDPREV);
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				LONG currentWindowStyle = ::GetWindowLongW(currentHWND, GWL_STYLE);
				if (!(currentWindowStyle & WS_MINIMIZE))
					{
					if (currentTopLevel->Internal_GetZLevel() < inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = previousHWND;
						}
					else if (!currentTopLevel->Internal_CanMoveWithinLevel()
						&& currentTopLevel->Internal_GetZLevel() == inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = previousHWND;
						}
					}
				}
			currentHWND = previousHWND;
			}
		}
	return realHWNDAfter;
	}

static HWND sHandleWindowPosChangingA(ZOSWindow_TopLevel* inWindow, HWND inRequestedAfter)
	{
	// Two cases, either the window is being moved higher up the stack, or lower.

	// First scan from top to bottom till we hit our window. If we don't hit the inRequestedAfter
	// window before hitting our window then don't do anything -- that implies our window is
	// being moved lower in the stack
	bool hitRequestedAfter = false;
	if (inRequestedAfter == HWND_TOPMOST)
		hitRequestedAfter = true;
	else if (inRequestedAfter == HWND_BOTTOM)
		inRequestedAfter = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);

	HWND realHWNDAfter = inRequestedAfter;
	HWND currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDFIRST);
	while (currentHWND && currentHWND != inWindow->GetHWND())
		{
		HWND nextHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		if (inRequestedAfter == HWND_TOP
			&& ((::GetWindowLongA(currentHWND, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0))
			{
			hitRequestedAfter = true;
			}

		if (currentHWND == inRequestedAfter)
			hitRequestedAfter = true;

		if (hitRequestedAfter)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				LONG currentWindowStyle = ::GetWindowLongA(currentHWND, GWL_STYLE);
				if (!(currentWindowStyle & WS_MINIMIZE))
					{
					if (currentTopLevel->Internal_GetZLevel() > inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = currentHWND;
						}
					else if (!currentTopLevel->Internal_CanMoveWithinLevel()
						&& currentTopLevel->Internal_GetZLevel() == inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = currentHWND;
						}
					}
				}
			}
		currentHWND = nextHWND;
		}

	if (!hitRequestedAfter)
		{
		// We must be moving the window down the stack, so start at the bottom
		// and move each window in a lower layer so it is behind inRequestedAfter
		currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);
		while (currentHWND && currentHWND != inWindow->GetHWND())
			{
			HWND previousHWND = ::GetWindow(currentHWND, GW_HWNDPREV);
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				LONG currentWindowStyle = ::GetWindowLongA(currentHWND, GWL_STYLE);
				if (!(currentWindowStyle & WS_MINIMIZE))
					{
					if (currentTopLevel->Internal_GetZLevel() < inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = previousHWND;
						}
					else if (!currentTopLevel->Internal_CanMoveWithinLevel()
						&& currentTopLevel->Internal_GetZLevel() == inWindow->Internal_GetZLevel())
						{
						::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
							SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
						realHWNDAfter = previousHWND;
						}
					}
				}
			currentHWND = previousHWND;
			}
		}
	return realHWNDAfter;
	}

LRESULT ZOSWindow_TopLevel::WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_BringFront)
		{
		sAllowActivationChange = true;
		::MySetWindowPos(fHWND, HWND_TOP, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		sAllowActivationChange = false;

		HWND topmostModal = ::sGetTopmostVisibleModal();
		if (topmostModal)
			::SetForegroundWindow(topmostModal);
		else
			::SetForegroundWindow(fHWND);
		return 0;
		}

	switch (inMessage)
		{
// ----------
// Lifetime
		case WM_NCDESTROY:
			{
			LRESULT result = ZOSWindow_Win::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			if (fHWND_TaskBarDummy)
				{
				::DestroyWindow(fHWND_TaskBarDummy);
				fHWND_TaskBarDummy = nil;
				}
			return result;
			}
// ----------
// Non client events and mouse events
		case WM_NCACTIVATE:
			{
			bool callBase = true;
			bool allowChange = true;

			// If we can't be targetted and we're being asked to dehilight, then
			// don't call the base proc
			if (!this->Internal_CanBeTargetted() && inWPARAM == 0)
				callBase = false;

			if (!sAllowActivationChange)
				{
				if (inWPARAM == 0)
					{
					ZOSWindow_TopLevel* otherTopLevel = dynamic_cast<ZOSWindow_TopLevel*>
						(ZOSWindow_Win::sFromHWNDNilOkay(reinterpret_cast<HWND>(inLPARAM)));

					if (sDisallowActivationChange)
						{
						allowChange = false;
						callBase = false;
						}

					if (otherTopLevel && this->Internal_IsModal())
						{
						HWND topmostModal = ::sGetTopmostVisibleModal();
						if (topmostModal != reinterpret_cast<HWND>(inLPARAM))
							{
							allowChange = false;
							callBase = false;
							}
						}
					if (otherTopLevel && !otherTopLevel->Internal_CanBeTargetted())
						{
						allowChange = false;
						callBase = false;
						}
					}
				}
			if (callBase)
				ZOSWindow_Win::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			return allowChange;
			}
		case WM_ACTIVATEAPP:
			{
			#if 0
				if (fHideOnSuspend)
					{
					bool isCurrentlyVisible = ::IsWindowVisible(fHWND);
					if (inWPARAM)
						{
						// Resuming
						fSuspended = false;
						if (fVisibleBeforeSuspend && !isCurrentlyVisible)
							{
							::MySetWindowPos(fHWND, nil, 0, 0, 0, 0,
								SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING
								| SWP_NOZORDER | SWP_SHOWWINDOW);
							}
						}
					else
						{
						// Suspending
						fVisibleBeforeSuspend = isCurrentlyVisible;
						fSuspended = true;
						if (isCurrentlyVisible)
							{
							::MySetWindowPos(fHWND, nil, 0, 0, 0, 0,
								SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING
								| SWP_NOZORDER | SWP_HIDEWINDOW);
							}
						}
					}
			#endif
			break;
			}
		case WM_WINDOWPOSCHANGING:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			WINDOWPOS* theWINDOWPOS = reinterpret_cast<WINDOWPOS*>(inLPARAM);
			if (sDisallowActivationChange)
				theWINDOWPOS->flags |= SWP_NOZORDER | SWP_NOACTIVATE;

			if (ZUtil_Win::sUseWAPI())
				{
				bool isMinimized = (::GetWindowLongW(fHWND, GWL_STYLE) & WS_MINIMIZE) != 0;
				bool isMinimizing = (theWINDOWPOS->flags & 0x8000) != 0 && isMinimized;

				if ((theWINDOWPOS->flags & SWP_NOZORDER) == 0 && !isMinimizing)
					{
					theWINDOWPOS->hwndInsertAfter
						= sHandleWindowPosChangingW(this, theWINDOWPOS->hwndInsertAfter);

					if (theWINDOWPOS->hwndInsertAfter == theWINDOWPOS->hwnd)
						theWINDOWPOS->flags |= SWP_NOZORDER;
					}
				}
			else
				{
				bool isMinimized = (::GetWindowLongA(fHWND, GWL_STYLE) & WS_MINIMIZE) != 0;
				bool isMinimizing = (theWINDOWPOS->flags & 0x8000) != 0 && isMinimized;

				if ((theWINDOWPOS->flags & SWP_NOZORDER) == 0 && !isMinimizing)
					{
					theWINDOWPOS->hwndInsertAfter
						= sHandleWindowPosChangingA(this, theWINDOWPOS->hwndInsertAfter);

					if (theWINDOWPOS->hwndInsertAfter == theWINDOWPOS->hwnd)
						theWINDOWPOS->flags |= SWP_NOZORDER;
					}
				}

			return result;
			}
		case WM_SYSCOMMAND:
			{
			switch (inWPARAM & 0xFFF0)
				{
				case SC_MINIMIZE:
					{
					if (fAppearsOnTaskBar)
						::ShowWindow(fHWND, SW_MINIMIZE); 
					return 0;
					}
				case SC_MAXIMIZE:
					{
					::ShowWindow(fHWND, SW_MAXIMIZE); 
					return 0;
					}
				case SC_RESTORE:
					{
					::ShowWindow(fHWND, SW_RESTORE); 
					return 0;
					}
				case SC_CLOSE:
					{
					HWND topmostModal = ::sGetTopmostVisibleModal();
					if (topmostModal == nil || topmostModal == fHWND)
						::sSendMessage(fHWND, WM_CLOSE, 0, 0);
					return 0;
					}
				case SC_MOVE:
					{
					bool priorDisallowActivationChange = sDisallowActivationChange;
					sDisallowActivationChange = true;
					LRESULT result = ZOSWindow_Win::WindowProc(inHWND, inMessage,
						inWPARAM, inLPARAM);
					sDisallowActivationChange = priorDisallowActivationChange;
					return result;
					}
				case SC_SIZE:
					{
					bool priorDisallowActivationChange = sDisallowActivationChange;
					sDisallowActivationChange = true;
					LRESULT result = ZOSWindow_Win::WindowProc(inHWND, inMessage,
						inWPARAM, inLPARAM);
					sDisallowActivationChange = priorDisallowActivationChange;
					return result;
					}
				}
			break;
			}
		}
	return ZOSWindow_Win::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_SDI

void* ZOSWindow_SDI::sCreate(void* inParam)
	{
	pair<ZOSWindow_SDI*, const CreationAttributes*>* thePair
		= reinterpret_cast<pair<ZOSWindow_SDI*, const CreationAttributes*>*>(inParam);

	if (ZUtil_Win::sUseWAPI())
		thePair->first->CreateW(*thePair->second);
	else
		thePair->first->CreateA(*thePair->second);
	return nil;
	}

void ZOSWindow_SDI::CreateW(const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const UTF16 windowClassName[] = L"ZOSWindow_Win::sWindowProcW";
	::sRegisterWindowClassW(windowClassName, ZOSWindow_Win::sWindowProcW);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, fHasMenuBar, theExWindowStyle);

	if (!fAppearsOnTaskBar)
		{
		// Only need to create the dummy HWND if our window is not itself a tool window
		if (!(theExWindowStyle & WS_EX_TOOLWINDOW))
			{
			fHWND_TaskBarDummy = ::CreateWindowExW(
				WS_EX_TOOLWINDOW, // Extended attributes
				sWNDCLASSName_TaskBarDummyW, // window class name
				L"TaskBarDummy", // window caption
				WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				nil,
				(HMENU)nil,
				::GetModuleHandleW(nil), // program instance handle
				nil); // creation parameters
			}
		}

	HMENU theHMENU = nil;
	if (fHasMenuBar)
		{
		theHMENU = ::CreateMenu();
		::AppendMenuW(theHMENU, MF_STRING, 0, L"empty");
		}

	HWND theHWND = ::CreateWindowExW(
		theExWindowStyle, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		theWindowStyle | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		fHWND_TaskBarDummy, // Parent window
		theHMENU,
		::GetModuleHandleW(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);

	// Move the window to the top of its layer, without activating or showing it
	HWND realHWNDAfter = HWND_NOTOPMOST;
	HWND currentHWND = ::GetWindow(fHWND, GW_HWNDFIRST);
	while (currentHWND)
		{
		if (currentHWND != fHWND)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				if (currentTopLevel->Internal_GetZLevel() > this->Internal_GetZLevel())
					realHWNDAfter = currentTopLevel->GetHWND();
				}
			}
		currentHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		}
	::MySetWindowPos(fHWND, realHWNDAfter, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);

	if (!fCanBeTargetted)
		::sSendMessage(fHWND, WM_NCACTIVATE, 1, 0);
	}

void ZOSWindow_SDI::CreateA(const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const char windowClassName[] = "ZOSWindow_Win::sWindowProcA";
	::sRegisterWindowClassA(windowClassName, ZOSWindow_Win::sWindowProcA);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, fHasMenuBar, theExWindowStyle);

	if (!fAppearsOnTaskBar)
		{
		// Only need to create the dummy HWND if our window is not itself a tool window
		if (!(theExWindowStyle & WS_EX_TOOLWINDOW))
			{
			fHWND_TaskBarDummy = ::CreateWindowExA(
				WS_EX_TOOLWINDOW, // Extended attributes
				sWNDCLASSName_TaskBarDummyA, // window class name
				"TaskBarDummy", // window caption
				WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				nil,
				(HMENU)nil,
				::GetModuleHandleA(nil), // program instance handle
				nil); // creation parameters
			}
		}

	HMENU theHMENU = nil;
	if (fHasMenuBar)
		{
		theHMENU = ::CreateMenu();
		::AppendMenuA(theHMENU, MF_STRING, 0, "empty");
		}

	HWND theHWND = ::CreateWindowExA(
		theExWindowStyle, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		theWindowStyle | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		fHWND_TaskBarDummy, // Parent window
		theHMENU,
		::GetModuleHandleA(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);

	// Move the window to the top of its layer, without activating or showing it
	HWND realHWNDAfter = HWND_NOTOPMOST;
	HWND currentHWND = ::GetWindow(fHWND, GW_HWNDFIRST);
	while (currentHWND)
		{
		if (currentHWND != fHWND)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				if (currentTopLevel->Internal_GetZLevel() > this->Internal_GetZLevel())
					realHWNDAfter = currentTopLevel->GetHWND();
				}
			}
		currentHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		}
	::MySetWindowPos(fHWND, realHWNDAfter, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
	if (!fCanBeTargetted)
		::sSendMessage(fHWND, WM_NCACTIVATE, 1, 0);
	}

ZOSWindow_SDI::ZOSWindow_SDI(const ZOSWindow::CreationAttributes& inAttributes)
:	ZOSWindow_TopLevel(inAttributes.fHasSizeBox, inAttributes.fResizable,
		inAttributes.fHasCloseBox, inAttributes.fHasMenuBar, inAttributes.fLayer)
	{
	pair<ZOSWindow_SDI*, const CreationAttributes*> thePair(this, &inAttributes);
	ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCreate, &thePair);

	UINT theTimerID = ::SetTimer(fHWND, 1, 50, nil);

	RECT realFrame;
	::GetClientRect(fHWND, &realFrame);
	fSizeBoxFrame = realFrame;
	fSizeBoxFrame.left = fSizeBoxFrame.right - 13;
	fSizeBoxFrame.top = fSizeBoxFrame.bottom - 13;
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame));
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame) + 1);
	this->Internal_RecordFrameChange(realFrame);
	this->Internal_ReportFrameChange();
	}

ZOSWindow_SDI::~ZOSWindow_SDI()
	{
	if (ZDCCanvas_GDI_Window* tempCanvas = fCanvas)
		{
		fCanvas = nil;
		tempCanvas->OSWindowDisposed();
		}

	::sSendMessage(fHWND, sMSG_Destroy, 0, 0);
	ZAssertStop(kDebug_Win, fHWND == nil);
	}

bool ZOSWindow_SDI::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow_Win::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:WinMenuClick")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				ZMenu_Win::sPostSetup(fMenuBar, ::GetMenu(fHWND));
				::sPostMessage(fHWND, sMSG_Menu,
					inMessage.GetInt32("WPARAM"), inMessage.GetInt32("LPARAM"));
				}
			return true;
			}
		else if (theWhat == "zoolib:WinMenuKey")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				if (ZRef<ZMenuItem> theMenuItem
					= ZMenu_Win::sFindItemByAccel(fMenuBar,
					inMessage.GetInt32("virtualKey"),
					inMessage.GetBool("control"),
					inMessage.GetBool("shift"),
					inMessage.GetBool("alt")))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		}
	return false;
	}

void ZOSWindow_SDI::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	if (fHasMenuBar)
		{
		HMENU newHMENU = ZMenu_Win::sCreateHMENU(inMenuBar);
		::sSendMessage(fHWND, sMSG_InstallMenu, reinterpret_cast<WPARAM>(newHMENU), 0);
		}
	ZMutexLocker locker(fMutex_Structure);
	fMenuBar = inMenuBar;
	}

LRESULT ZOSWindow_SDI::CallBaseWindowProc(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::DefWindowProcW(inHWND, inMessage, inWPARAM, inLPARAM);
	else
		return ::DefWindowProcA(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_SDI::WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_InstallMenu)
		{
		HMENU oldHMENU = ::GetMenu(fHWND);
		::SetMenu(fHWND, reinterpret_cast<HMENU>(inWPARAM));
		::DrawMenuBar(fHWND);
		::DestroyMenu(oldHMENU);
		return 0;
		}

	switch (inMessage)
		{
		case WM_COMMAND:
			{
			if (fHasMenuBar)
				{
				if (ZRef<ZMenuItem> theMenuItem
					= ZMenu_Win::sFindItemByCommand(::GetMenu(fHWND), LOWORD(inWPARAM)))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->QueueMessageForOwner(theMessage);
					return 0;
					}
				}
			break;
			}
		case WM_SYSCOMMAND:
			{
			switch (inWPARAM & 0xFFF0)
				{
				case SC_MOUSEMENU:
				case SC_KEYMENU:
					{
					HWND topmostModal = ::sGetTopmostVisibleModal();
					if (topmostModal == nil || topmostModal == fHWND)
						{
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:WinMenuClick");
						theMessage.SetInt32("WPARAM", inWPARAM);
						theMessage.SetInt32("LPARAM", inLPARAM);
						this->QueueMessage(theMessage);
						}
					return 0;
					}
				}
			}
		}
	return ZOSWindow_TopLevel::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Frame

void* ZOSWindow_Frame::sCreate(void* inParam)
	{
	pair<ZOSWindow_Frame*, const CreationAttributes*>* thePair
		= reinterpret_cast<pair<ZOSWindow_Frame*, const CreationAttributes*>*>(inParam);

	if (ZUtil_Win::sUseWAPI())
		thePair->first->CreateW(*thePair->second);
	else
		thePair->first->CreateA(*thePair->second);
	return nil;
	}

void ZOSWindow_Frame::CreateW(const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const UTF16 windowClassName[] = L"ZOSWindow_Win::sWindowProcW";
	::sRegisterWindowClassW(windowClassName, ZOSWindow_Win::sWindowProcW);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, fHasMenuBar, theExWindowStyle);

	HMENU theHMENU = ::CreateMenu();
	::AppendMenuW(theHMENU, MF_STRING, 0, L"empty");

	HWND theHWND = ::CreateWindowExW(
		theExWindowStyle, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		theWindowStyle | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		nil,
		theHMENU,
		::GetModuleHandleW(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);

	// Move the window to the top of its layer, without activating or showing it
	HWND realHWNDAfter = HWND_NOTOPMOST;
	HWND currentHWND = ::GetWindow(fHWND, GW_HWNDFIRST);
	while (currentHWND)
		{
		if (currentHWND != fHWND)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				if (currentTopLevel->Internal_GetZLevel() > this->Internal_GetZLevel())
					realHWNDAfter = currentTopLevel->GetHWND();
				}
			}
		currentHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		}
	::MySetWindowPos(fHWND, realHWNDAfter, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
	}

void ZOSWindow_Frame::CreateA(const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const char windowClassName[] = "ZOSWindow_Win::sWindowProcA";
	::sRegisterWindowClassA(windowClassName, ZOSWindow_Win::sWindowProcA);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, fHasMenuBar, theExWindowStyle);

	HMENU theHMENU = ::CreateMenu();
	::AppendMenuA(theHMENU, MF_STRING, 0, "empty");

	HWND theHWND = ::CreateWindowExA(
		theExWindowStyle, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		theWindowStyle | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		nil,
		theHMENU,
		::GetModuleHandleA(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);

	// Move the window to the top of its layer, without activating or showing it
	HWND realHWNDAfter = HWND_NOTOPMOST;
	HWND currentHWND = ::GetWindow(fHWND, GW_HWNDFIRST);
	while (currentHWND)
		{
		if (currentHWND != fHWND)
			{
			if (ZOSWindow_TopLevel* currentTopLevel
				= dynamic_cast<ZOSWindow_TopLevel*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				if (currentTopLevel->Internal_GetZLevel() > this->Internal_GetZLevel())
					realHWNDAfter = currentTopLevel->GetHWND();
				}
			}
		currentHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		}
	::MySetWindowPos(fHWND, realHWNDAfter, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
	}

ZOSWindow_Frame::ZOSWindow_Frame(const ZOSWindow::CreationAttributes& inAttributes)
:	ZOSWindow_TopLevel(inAttributes.fHasSizeBox, inAttributes.fResizable,
		inAttributes.fHasCloseBox, inAttributes.fHasMenuBar, inAttributes.fLayer)
	{
	fHWND_Client = nil;
	fClientInsetTopLeft = ZPoint::sZero;
	fClientInsetBottomRight = ZPoint::sZero;

	pair<ZOSWindow_Frame*, const CreationAttributes*> thePair(this, &inAttributes);
	ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCreate, &thePair);

	UINT theTimerID = ::SetTimer(fHWND, 1, 50, nil);

	RECT realFrame;
	::GetClientRect(fHWND, &realFrame);
	fSizeBoxFrame = realFrame;
	fSizeBoxFrame.left = fSizeBoxFrame.right - 13;
	fSizeBoxFrame.top = fSizeBoxFrame.bottom - 13;
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame));
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame) + 1);
	this->Internal_RecordFrameChange(realFrame);
	this->Internal_ReportFrameChange();
	}

ZOSWindow_Frame::~ZOSWindow_Frame()
	{
	if (ZDCCanvas_GDI_Window* tempCanvas = fCanvas)
		{
		fCanvas = nil;
		tempCanvas->OSWindowDisposed();
		}

	::sSendMessage(fHWND, sMSG_Destroy, 0, 0);
	ZAssertStop(kDebug_Win, fHWND == nil);
	}

void ZOSWindow_Frame::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	}

LRESULT ZOSWindow_Frame::CallBaseWindowProc(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::DefFrameProcW(inHWND, fHWND_Client, inMessage, inWPARAM, inLPARAM);
	else
		return ::DefFrameProcA(inHWND, fHWND_Client, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_Frame::WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_MDIMenuBarChanged)
		{
		if (ZUtil_Win::sUseWAPI())
			{
			HWND activeWindow = reinterpret_cast<HWND>
				(::SendMessageW(fHWND_Client, WM_MDIGETACTIVE, 0, 0));

			if (inWPARAM == ::SendMessageW(fHWND_Client, WM_MDIGETACTIVE, 0, 0))
				{
				if (ZOSWindow_MDI* theWindow = dynamic_cast<ZOSWindow_MDI*>
					(sFromHWNDNilOkayW(reinterpret_cast<HWND>(inWPARAM))))
					{
					HMENU newHMENU = theWindow->GetHMENU();
					HMENU oldHMENU = (HMENU)::SendMessageW(fHWND_Client, WM_MDISETMENU,
						reinterpret_cast<WPARAM>(newHMENU), 0);
					::DrawMenuBar(fHWND);
					::DestroyMenu(oldHMENU);
					}
				}
			}
		else
			{
			HWND activeWindow = reinterpret_cast<HWND>
				(::SendMessageA(fHWND_Client, WM_MDIGETACTIVE, 0, 0));

			if (inWPARAM == ::SendMessageA(fHWND_Client, WM_MDIGETACTIVE, 0, 0))
				{
				if (ZOSWindow_MDI* theWindow = dynamic_cast<ZOSWindow_MDI*>
					(sFromHWNDNilOkayA(reinterpret_cast<HWND>(inWPARAM))))
					{
					HMENU newHMENU = theWindow->GetHMENU();
					HMENU oldHMENU = (HMENU)::SendMessageA(fHWND_Client, WM_MDISETMENU,
						reinterpret_cast<WPARAM>(newHMENU), 0);
					::DrawMenuBar(fHWND);
					::DestroyMenu(oldHMENU);
					}
				}
			}
		return 0;
		}
	else if (inMessage == sMSG_MDIActivated)
		{
		if (ZUtil_Win::sUseWAPI())
			{
			if (inLPARAM == 0)
				{
				ZMutexLocker locker(fMutex_Structure);
				HMENU newHMENU = ::CreateMenu();
				::AppendMenuW(newHMENU, MF_STRING, 0, L"nowindows");
				locker.Release();
				HMENU oldHMENU = (HMENU)::SendMessageW(fHWND_Client, WM_MDISETMENU,
					reinterpret_cast<WPARAM>(newHMENU), 0);
				::DrawMenuBar(fHWND);
				::DestroyMenu(oldHMENU);
				}
			else
				{
				if (ZOSWindow_MDI* theWindow = dynamic_cast<ZOSWindow_MDI*>
					(sFromHWNDNilOkayW(reinterpret_cast<HWND>(inLPARAM))))
					{
					HMENU newHMENU = theWindow->GetHMENU();
					HMENU oldHMENU = (HMENU)::SendMessageW(fHWND_Client, WM_MDISETMENU,
						reinterpret_cast<WPARAM>(newHMENU), 0);
					::DrawMenuBar(fHWND);
					::DestroyMenu(oldHMENU);
					}
				}
			}
		else
			{
			if (inLPARAM == 0)
				{
				ZMutexLocker locker(fMutex_Structure);
				HMENU newHMENU = ::CreateMenu();
				::AppendMenuA(newHMENU, MF_STRING, 0, "nowindows");
				locker.Release();
				HMENU oldHMENU = (HMENU)::SendMessageA(fHWND_Client, WM_MDISETMENU,
					reinterpret_cast<WPARAM>(newHMENU), 0);
				::DrawMenuBar(fHWND);
				::DestroyMenu(oldHMENU);
				}
			else
				{
				if (ZOSWindow_MDI* theWindow = dynamic_cast<ZOSWindow_MDI*>
					(sFromHWNDNilOkayA(reinterpret_cast<HWND>(inLPARAM))))
					{
					HMENU newHMENU = theWindow->GetHMENU();
					HMENU oldHMENU = (HMENU)::sSendMessage(fHWND_Client, WM_MDISETMENU,
						reinterpret_cast<WPARAM>(newHMENU), 0);
					::DrawMenuBar(fHWND);
					::DestroyMenu(oldHMENU);
					}
				}
			}
		return 0;
		}

	switch (inMessage)
		{
		case WM_CREATE:
			{
			CLIENTCREATESTRUCT clientcreate;
			clientcreate.hWindowMenu = nil;
			// Start child IDs at a fairly large number
			clientcreate.idFirstChild = 31000;
			RECT clientRect;
			::GetClientRect(fHWND, &clientRect);
			if (ZUtil_Win::sUseWAPI())
				{
				fHWND_Client = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"mdiclient", nil,
					WS_HSCROLL | WS_VSCROLL | WS_CHILD
					| WS_CLIPCHILDREN | WS_VISIBLE | MDIS_ALLCHILDSTYLES,
					clientRect.left,
					clientRect.top,
					clientRect.right - clientRect.left,
					clientRect.bottom - clientRect.top,
					fHWND, nil, ::GetModuleHandleW(nil),
					&clientcreate);

				// Attach the marker property
				::SetPropW(fHWND_Client, L"ZOSWindow_Frame", reinterpret_cast<HANDLE>(this));
				// Set the user data
				::SetWindowLongW(fHWND_Client, GWL_USERDATA, reinterpret_cast<LONG>(this));

				// And finally install the replacement window proc
				fWNDPROC_Client = reinterpret_cast<WNDPROC>
					(::GetWindowLongW(fHWND_Client, GWL_WNDPROC));

				::SetWindowLongW(fHWND_Client, GWL_WNDPROC,
					reinterpret_cast<LONG>(sWindowProc_ClientW));
				}
			else
				{
				fHWND_Client = ::CreateWindowExA(WS_EX_CLIENTEDGE, "mdiclient", nil,
					WS_HSCROLL | WS_VSCROLL | WS_CHILD
					| WS_CLIPCHILDREN | WS_VISIBLE | MDIS_ALLCHILDSTYLES,
					clientRect.left,
					clientRect.top,
					clientRect.right - clientRect.left,
					clientRect.bottom - clientRect.top,
					fHWND, nil, ::GetModuleHandleA(nil),
					&clientcreate);

				// Attach the marker property
				::SetPropA(fHWND_Client, "ZOSWindow_Frame", reinterpret_cast<HANDLE>(this));
				// Set the user data
				::SetWindowLongA(fHWND_Client, GWL_USERDATA, reinterpret_cast<LONG>(this));

				// And finally install the replacement window proc
				fWNDPROC_Client = reinterpret_cast<WNDPROC>
					(::GetWindowLongA(fHWND_Client, GWL_WNDPROC));

				::SetWindowLongA(fHWND_Client, GWL_WNDPROC,
					reinterpret_cast<LONG>(sWindowProc_ClientA));
				}
			break;
			}
		case WM_SIZE:
			{
			// DefFrameProc will resize our client HWND to fill our client area,
			// whereas we want it to respect our fClientInset values, so we handle
			// WM_SIZE without calling DefFrameProc.
			if (fHWND_Client)
				{
				::SetWindowPos(fHWND_Client, nil,
					fClientInsetTopLeft.h, fClientInsetTopLeft.v,
					(short(LOWORD(inLPARAM))) - fClientInsetTopLeft.h + fClientInsetBottomRight.h,
					(short(HIWORD(inLPARAM))) - fClientInsetTopLeft.v + fClientInsetBottomRight.v,
					SWP_NOZORDER);

				return 0;
				}
			}
		case WM_COMMAND:
			{
			if (fHasMenuBar)
				{
				if (ZRef<ZMenuItem> theMenuItem =
					ZMenu_Win::sFindItemByCommand(::GetMenu(fHWND), LOWORD(inWPARAM)))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					if (ZOSWindow_MDI* activeWindow = this->GetActiveOSWindow_MDI())
						activeWindow->QueueMessage(theMessage);
					else
						this->QueueMessage(theMessage);
					return 0;
					}
				}
			break;
			}
		case WM_SYSCOMMAND:
			{
			switch (inWPARAM & 0xFFF0)
				{
				case SC_MOUSEMENU:
				case SC_KEYMENU:
					{
					HWND topmostModal = ::sGetTopmostVisibleModal();
					if (topmostModal == nil || topmostModal == fHWND)
						{
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:WinMenuClick");
						theMessage.SetInt32("WPARAM", inWPARAM);
						theMessage.SetInt32("LPARAM", inLPARAM);

						if (ZOSWindow_MDI* activeWindow = this->GetActiveOSWindow_MDI())
							activeWindow->QueueMessage(theMessage);
						else
							this->QueueMessage(theMessage);
						}
					return 0;
					}
				}
			}
		}
	return ZOSWindow_TopLevel::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

ZOSWindow_MDI* ZOSWindow_Frame::CreateOSWindow_MDI(
	const ZOSWindow::CreationAttributes& inAttributes)
	{ return new ZOSWindow_MDI(fHWND_Client, inAttributes); }

ZOSWindow_MDI* ZOSWindow_Frame::GetActiveOSWindow_MDI()
	{
	if (ZUtil_Win::sUseWAPI())
		{
		if (HWND theHWND = reinterpret_cast<HWND>
			(::SendMessageW(fHWND_Client, WM_MDIGETACTIVE, 0, 0)))
			{
			return dynamic_cast<ZOSWindow_MDI*>(sFromHWNDNilOkayW(theHWND));
			}
		}
	else
		{
		if (HWND theHWND = reinterpret_cast<HWND>
			(::SendMessageA(fHWND_Client, WM_MDIGETACTIVE, 0, 0)))
			{
			return dynamic_cast<ZOSWindow_MDI*>(sFromHWNDNilOkayA(theHWND));
			}
		}
	return nil;
	}

LRESULT ZOSWindow_Frame::WindowProc_ClientW(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	switch (inMessage)
		{
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
			::SendMessageW(fHWND, sMSG_BringFront, 0, 0);
			break;
			}
		}
	return ::CallWindowProcW(fWNDPROC_Client, inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT ZOSWindow_Frame::WindowProc_ClientA(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	switch (inMessage)
		{
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
			::SendMessageA(fHWND, sMSG_BringFront, 0, 0);
			break;
			}
		}
	return ::CallWindowProcA(fWNDPROC_Client, inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT CALLBACK ZOSWindow_Frame::sWindowProc_ClientW(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Frame* theWindow_Frame
		= reinterpret_cast<ZOSWindow_Frame*>(::GetWindowLongW(inHWND, GWL_USERDATA));

	return theWindow_Frame->WindowProc_ClientW(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT CALLBACK ZOSWindow_Frame::sWindowProc_ClientA(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	ZOSWindow_Frame* theWindow_Frame
		= reinterpret_cast<ZOSWindow_Frame*>(::GetWindowLongA(inHWND, GWL_USERDATA));

	return theWindow_Frame->WindowProc_ClientA(inHWND, inMessage, inWPARAM, inLPARAM);
	}

void ZOSWindow_Frame::SetClientInsets(ZPoint inClientInsetTopLeft, ZPoint inClientInsetBottomRight)
	{
	if (fClientInsetTopLeft == inClientInsetTopLeft
		&& fClientInsetBottomRight == inClientInsetBottomRight)
		{
		return;
		}

	fClientInsetTopLeft = inClientInsetTopLeft;
	fClientInsetBottomRight = inClientInsetBottomRight;
	if (fHWND_Client)
		{
		RECT clientRect;
		::GetClientRect(fHWND, &clientRect);
		::SetWindowPos(fHWND_Client, nil,
					clientRect.left + fClientInsetTopLeft.h,
					clientRect.top + fClientInsetTopLeft.v,
					clientRect.right - clientRect.left
						- fClientInsetTopLeft.h + fClientInsetBottomRight.h,
					clientRect.bottom - clientRect.top
						- fClientInsetTopLeft.v + fClientInsetBottomRight.v,
					SWP_NOZORDER);

		}
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Frame::GetClientInsets(ZPoint& outClientInsetTopLeft,
	ZPoint& outClientInsetBottomRight)
	{
	outClientInsetTopLeft = fClientInsetTopLeft;
	outClientInsetBottomRight = fClientInsetBottomRight;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_MDI

struct MDICreateParams
	{
	ZOSWindow_MDI* fOSWindow_MDI;
	HWND fHWND_Client;
	const ZOSWindow::CreationAttributes* fAttributes;
	};

void* ZOSWindow_MDI::sCreate(void* inParam)
	{
	MDICreateParams* theParams = reinterpret_cast<MDICreateParams*>(inParam);
	if (ZUtil_Win::sUseWAPI())
		theParams->fOSWindow_MDI->CreateW(theParams->fHWND_Client, *theParams->fAttributes);
	else
		theParams->fOSWindow_MDI->CreateA(theParams->fHWND_Client, *theParams->fAttributes);
	return nil;
	}

void ZOSWindow_MDI::CreateW(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const UTF16 windowClassName[] = L"ZOSWindow_Win::sWindowProc_MDIW";
	::sRegisterWindowClassW(windowClassName, ZOSWindow_Win::sWindowProc_MDIW);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, false, theExWindowStyle);

	HWND theHWND = ::CreateWindowExW(
		theExWindowStyle | WS_EX_MDICHILD, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		(theWindowStyle& ~WS_POPUP) | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		inHWND_Client,
		(HMENU)nil,
		::GetModuleHandleW(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);
	}

void ZOSWindow_MDI::CreateA(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes)
	{
	DWORD theWindowStyle;
	DWORD theExWindowStyle;

	ZOSWindow_Win::sFromCreationAttributes(inAttributes, theWindowStyle, theExWindowStyle);
	static const char windowClassName[] = "ZOSWindow_Win::sWindowProc_MDIA";
	::sRegisterWindowClassA(windowClassName, ZOSWindow_Win::sWindowProc_MDIA);

	RECT realFrame = inAttributes.fFrame;
	::AdjustWindowRectEx(&realFrame, theWindowStyle, false, theExWindowStyle);

	HWND theHWND = ::CreateWindowExA(
		theExWindowStyle | WS_EX_MDICHILD, // Extended attributes
		windowClassName, // window class name
		nil, // window caption
		(theWindowStyle& ~WS_POPUP) | WS_CLIPCHILDREN, // window style
		realFrame.left, // initial x position
		realFrame.top, // initial y position
		realFrame.right - realFrame.left, // initial x size
		realFrame.bottom - realFrame.top, // initial y size
		inHWND_Client,
		(HMENU)nil,
		::GetModuleHandleA(nil), // program instance handle
		this); // creation parameters

	ZAssertStop(kDebug_Win, fHWND != nil && fHWND == theHWND);
	}

ZOSWindow_MDI::ZOSWindow_MDI(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes)
:	ZOSWindow_Win(inAttributes.fHasSizeBox, inAttributes.fResizable, inAttributes.fHasCloseBox,
		inAttributes.fHasMenuBar, sLayerToZLevel(inAttributes.fLayer))
	{
	MDICreateParams theParams;
	theParams.fOSWindow_MDI = this;
	theParams.fHWND_Client = inHWND_Client;
	theParams.fAttributes = &inAttributes;
	ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCreate, &theParams);

	ZOSApp_Win::sGet()->AddOSWindow(this);

	UINT theTimerID = ::SetTimer(fHWND, 1, 50, nil);

	RECT realFrame;
	::GetClientRect(fHWND, &realFrame);
	fSizeBoxFrame = realFrame;
	fSizeBoxFrame.left = fSizeBoxFrame.right - 13;
	fSizeBoxFrame.top = fSizeBoxFrame.bottom - 13;
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame));
	::ScreenToClient(inHWND_Client, reinterpret_cast<POINT*>(&realFrame));
	::ClientToScreen(fHWND, reinterpret_cast<POINT*>(&realFrame) + 1);
	::ScreenToClient(inHWND_Client, reinterpret_cast<POINT*>(&realFrame) + 1);
	this->Internal_RecordFrameChange(realFrame);
	this->Internal_ReportFrameChange();
	}

ZOSWindow_MDI::~ZOSWindow_MDI()
	{
	ZOSApp_Win::sGet()->RemoveOSWindow(this);

	if (ZDCCanvas_GDI_Window* tempCanvas = fCanvas)
		{
		fCanvas = nil;
		tempCanvas->OSWindowDisposed();
		}

	HWND clientHWND = ::GetParent(fHWND);
	::sSendMessage(clientHWND, WM_MDIDESTROY, (WPARAM)fHWND, 0);
	ZAssertStop(kDebug_Win, fHWND == nil);
	}

bool ZOSWindow_MDI::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow_Win::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:WinMenuClick")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				HWND frameHWND = ::GetParent(::GetParent(fHWND));
				ZMenu_Win::sPostSetup(fMenuBar, ::GetMenu(frameHWND));
				::sPostMessage(frameHWND, sMSG_Menu,
					inMessage.GetInt32("WPARAM"), inMessage.GetInt32("LPARAM"));
				}
			return true;
			}
		else if (theWhat == "zoolib:WinMenuKey")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				if (ZRef<ZMenuItem> theMenuItem = ZMenu_Win::sFindItemByAccel(fMenuBar,
					inMessage.GetInt32("virtualKey"),
					inMessage.GetBool("control"),
					inMessage.GetBool("shift"),
					inMessage.GetBool("alt")))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		}
	return false;
	}

void ZOSWindow_MDI::Center()
	{ this->Center(true, true, false); }

void ZOSWindow_MDI::CenterDialog()
	{ this->Center(true, true, true); }

ZPoint ZOSWindow_MDI::ToGlobal(const ZPoint& inWindowPoint)
	{
	ASSERTLOCKED();
	// Remember that ToGlobal works using our *reported* bounds, not
	// whatever position the window currently occupies. There's a good
	// argument to support this approach, but it's a bit longwinded.
	POINT locationInClient = ZOSWindow_Win::ToGlobal(inWindowPoint);
	::ClientToScreen(::GetParent(fHWND), &locationInClient);
	return locationInClient;
	}

void ZOSWindow_MDI::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	ZMutexLocker locker(fMutex_Structure);
	ZMenuBar oldMenuBar = fMenuBar;
	fMenuBar = inMenuBar;
	locker.Release();

	HWND frameHWND = ::GetParent(::GetParent(fHWND));
	::sSendMessage(frameHWND, sMSG_MDIMenuBarChanged, reinterpret_cast<WPARAM>(fHWND), 0);
	}

LRESULT ZOSWindow_MDI::CallBaseWindowProc(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (ZUtil_Win::sUseWAPI())
		return ::DefMDIChildProcW(inHWND, inMessage, inWPARAM, inLPARAM);
	else
		return ::DefMDIChildProcA(inHWND, inMessage, inWPARAM, inLPARAM);
	}

static HWND sHandleWindowPosChangingW(ZOSWindow_MDI* inWindow, HWND inRequestedAfter)
	{
	bool hitRequestedAfter = false;
	if (inRequestedAfter == HWND_TOP)
		hitRequestedAfter = true;
	else if (inRequestedAfter == HWND_BOTTOM)
		inRequestedAfter = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);

	HWND currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDFIRST);
	HWND realHWNDAfter = inRequestedAfter;
	while (currentHWND && currentHWND != inWindow->GetHWND())
		{
		HWND nextHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		if (currentHWND == inRequestedAfter)
			hitRequestedAfter = true;
		if (hitRequestedAfter)
			{
			if (ZOSWindow_MDI* currentMDIWindow
				= dynamic_cast<ZOSWindow_MDI*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				if (currentMDIWindow->Internal_GetZLevel() > inWindow->Internal_GetZLevel())
					{
					::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
					realHWNDAfter = currentHWND;
					}
				}
			}
		currentHWND = nextHWND;
		}

	if (!hitRequestedAfter)
		{
		currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);
		while (currentHWND && currentHWND != inWindow->GetHWND())
			{
			HWND previousHWND = ::GetWindow(currentHWND, GW_HWNDPREV);
			if (ZOSWindow_MDI* currentMDIWindow
				= dynamic_cast<ZOSWindow_MDI*>(ZOSWindow_Win::sFromHWNDNilOkayW(currentHWND)))
				{
				if (currentMDIWindow->Internal_GetZLevel() < inWindow->Internal_GetZLevel())
					{
					::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
					realHWNDAfter = previousHWND;
					}
				}
			currentHWND = previousHWND;
			}
		}
	return realHWNDAfter;
	}

static HWND sHandleWindowPosChangingA(ZOSWindow_MDI* inWindow, HWND inRequestedAfter)
	{
	bool hitRequestedAfter = false;
	if (inRequestedAfter == HWND_TOP)
		hitRequestedAfter = true;
	else if (inRequestedAfter == HWND_BOTTOM)
		inRequestedAfter = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);

	HWND currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDFIRST);
	HWND realHWNDAfter = inRequestedAfter;
	while (currentHWND && currentHWND != inWindow->GetHWND())
		{
		HWND nextHWND = ::GetWindow(currentHWND, GW_HWNDNEXT);
		if (currentHWND == inRequestedAfter)
			hitRequestedAfter = true;
		if (hitRequestedAfter)
			{
			if (ZOSWindow_MDI* currentMDIWindow
				= dynamic_cast<ZOSWindow_MDI*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				if (currentMDIWindow->Internal_GetZLevel() > inWindow->Internal_GetZLevel())
					{
					::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
					realHWNDAfter = currentHWND;
					}
				}
			}
		currentHWND = nextHWND;
		}

	if (!hitRequestedAfter)
		{
		currentHWND = ::GetWindow(inWindow->GetHWND(), GW_HWNDLAST);
		while (currentHWND && currentHWND != inWindow->GetHWND())
			{
			HWND previousHWND = ::GetWindow(currentHWND, GW_HWNDPREV);
			if (ZOSWindow_MDI* currentMDIWindow
				= dynamic_cast<ZOSWindow_MDI*>(ZOSWindow_Win::sFromHWNDNilOkayA(currentHWND)))
				{
				if (currentMDIWindow->Internal_GetZLevel() < inWindow->Internal_GetZLevel())
					{
					::MySetWindowPos(currentHWND, realHWNDAfter, 0, 0, 0, 0,
						SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOSENDCHANGING);
					realHWNDAfter = previousHWND;
					}
				}
			currentHWND = previousHWND;
			}
		}
	return realHWNDAfter;
	}

LRESULT ZOSWindow_MDI::WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_BringFront)
		{
		::sSendMessage(::GetParent(::GetParent(fHWND)), sMSG_BringFront, 0, 0);
		::MySetWindowPos(fHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		return 0;
		}

	switch (inMessage)
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			{
			if ((::GetKeyState(VK_CONTROL) & 0x8000) && !(::GetKeyState(VK_MENU) & 0x8000))
				{
				switch (inWPARAM)
					{
					case VK_F6:
					case VK_TAB:
						{
						if (::GetKeyState(VK_SHIFT) & 0x8000)
							::sSendMessage(fHWND, WM_SYSCOMMAND, SC_NEXTWINDOW, inWPARAM);
						else
							::sSendMessage(fHWND, WM_SYSCOMMAND, SC_PREVWINDOW, inWPARAM);
						return 1;
						}
					case VK_F4:
					case VK_RBUTTON:
						{
						::sSendMessage(fHWND, WM_SYSCOMMAND, SC_CLOSE, inWPARAM);
						return 1;
						}
					}
				}
			break;
			}
		case WM_WINDOWPOSCHANGING:
			{
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			WINDOWPOS* theWINDOWPOS = reinterpret_cast<WINDOWPOS*>(inLPARAM);
			if ((theWINDOWPOS->flags & SWP_NOZORDER) == 0)
				{
				if (ZUtil_Win::sUseWAPI())
					{
					theWINDOWPOS->hwndInsertAfter
						= sHandleWindowPosChangingW(this, theWINDOWPOS->hwndInsertAfter);
					}
				else
					{
					theWINDOWPOS->hwndInsertAfter
						= sHandleWindowPosChangingA(this, theWINDOWPOS->hwndInsertAfter);
					}

				if (theWINDOWPOS->hwndInsertAfter == theWINDOWPOS->hwnd)
					theWINDOWPOS->flags |= SWP_NOZORDER;
				}
			return result;
			}
		case WM_MDIACTIVATE:
			{
			ZDebugLogf(kDebug_Win, ("WM_MDIACTIVATE, %X, %X, %X", fHWND, inWPARAM, inLPARAM));
			LRESULT result = this->CallBaseWindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
			if (fHWND == reinterpret_cast<HWND>(inLPARAM))
				{
				this->Internal_RecordActiveChange(true);
				HWND frameHWND = ::GetParent(::GetParent(fHWND));
				::sSendMessage(frameHWND, sMSG_MDIActivated, inWPARAM, inLPARAM);
				}
			else if (fHWND == reinterpret_cast<HWND>(inWPARAM))
				{
				this->Internal_RecordActiveChange(false);
				// Only send sMSG_MDIActivated if some other window is *not* being activated.
				// If there is another window, then *its* window proc will execute the lines
				// above and thus notify the frame window of the change in active MDI windows.
				if (inLPARAM == 0)
					{
					HWND frameHWND = ::GetParent(::GetParent(fHWND));
					::sSendMessage(frameHWND, sMSG_MDIActivated, inWPARAM, inLPARAM);
					}
				}
			return result;
			}
		}

	return ZOSWindow_Win::WindowProc(inHWND, inMessage, inWPARAM, inLPARAM);
	}

void ZOSWindow_MDI::Center(bool inHorizontal, bool inVertical, bool inForDialog)
	{
	ASSERTLOCKED();

	RECT windowRect;
	::GetWindowRect(fHWND, &windowRect);
	ZPoint windowSize(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);

	RECT mdiClientRECT;
	::GetClientRect(::GetParent(fHWND), &mdiClientRECT);
	ZRect mdiClientRect(mdiClientRECT);

	ZPoint location(ZPoint::sZero);
	if (inHorizontal)
		location.h = (mdiClientRect.Width() - windowSize.h)/2;

	if (inVertical)
		{
		// For a dialog, we have 2/3 of the space below the window and 1/3 above it.
		// For normal centering it's half and half
		if (inForDialog)
			location.v = (mdiClientRect.Height() - windowSize.v)/3;
		else
			location.v = (mdiClientRect.Height() - windowSize.v)/2;
		}
	// Put back the menu bar height
	location.v += mdiClientRect.top;

	// Put the window's upper left corner at location
	::MySetWindowPos(fHWND, nil, location.h, location.v, 0,0, SWP_NOSIZE | SWP_NOZORDER);
	this->Internal_ReportFrameChange();
	}

HMENU ZOSWindow_MDI::GetHMENU()
	{
	ZMutexLocker locker(fMutex_Structure);
	return ZMenu_Win::sCreateHMENU(fMenuBar);
	}

ZOSWindow_Frame* ZOSWindow_MDI::GetOSWindow_Frame()
	{
// Our parent is the client HWND
	HWND clientHWND = ::GetParent(fHWND);
// And its parent is the frame window
	HWND frameHWND = ::GetParent(clientHWND);
	ZOSWindow_Frame* theOSWindow_Frame;
	if (ZUtil_Win::sUseWAPI())
		theOSWindow_Frame = dynamic_cast<ZOSWindow_Frame*>(sFromHWNDNilOkayW(frameHWND));
	else
		theOSWindow_Frame = dynamic_cast<ZOSWindow_Frame*>(sFromHWNDNilOkayA(frameHWND));
	ZAssertStop(kDebug_Win, theOSWindow_Frame);

	return theOSWindow_Frame;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Win::DragInitiator

static UINT sMSG_InitiateDrag = ::sRegisterWindowMessage("ZOSApp_Win::sMSG_InitiateDrag");

struct DragInitiateInfo
	{
	ZTuple fTuple;
	ZPoint fStartPoint;
	ZOSWindow_Win_DragFeedbackCombo fFeedbackCombo;
	ZDragSource* fDragSource;
	};

class ZOSApp_Win::DragInitiator : public ZDragInitiator
	{
public:
	DragInitiator(HWND inHWND_Utility);
	~DragInitiator();

// From ZDragInitiator
	virtual void Imp_DoDrag(const ZTuple& inTuple,
				ZPoint inStartPoint, ZPoint inHotPoint,
				const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap,
				const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap,
				ZDragSource* inDragSource);

protected:
	HWND fHWND_Utility;
	};

ZOSApp_Win::DragInitiator::DragInitiator(HWND inHWND_Utility)
:	fHWND_Utility(inHWND_Utility)
	{}

ZOSApp_Win::DragInitiator::~DragInitiator()
	{}

void ZOSApp_Win::DragInitiator::Imp_DoDrag(const ZTuple& inTuple,
	ZPoint inStartPoint, ZPoint inHotPoint,
	const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap,
	const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap,
	ZDragSource* inDragSource)
	{
	DragInitiateInfo* theDragInitiateInfo = new DragInitiateInfo;

	theDragInitiateInfo->fTuple = inTuple;
	theDragInitiateInfo->fStartPoint = inStartPoint;
	theDragInitiateInfo->fFeedbackCombo.fHotPoint = inHotPoint;
	if (inDragRgn)
		theDragInitiateInfo->fFeedbackCombo.fRgn = *inDragRgn;

	if (inDragMaskPixmap && *inDragMaskPixmap)
		theDragInitiateInfo->fFeedbackCombo.fMaskRgn = ZRect(inDragMaskPixmap->Size());
	else if (inDragMaskRgn)
		theDragInitiateInfo->fFeedbackCombo.fMaskRgn = *inDragMaskRgn;

	theDragInitiateInfo->fDragSource = inDragSource;

	::sPostMessage(fHWND_Utility, sMSG_InitiateDrag,
		0, reinterpret_cast<LPARAM>(theDragInitiateInfo));
	delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Win

static UINT sMSG_InvokeFunction = ::sRegisterWindowMessage("ZOSApp_Win::sMSG_InvokeFunction");

ZOSApp_Win* ZOSApp_Win::sOSApp_Win = nil;

ZOSApp_Win* ZOSApp_Win::sGet()
	{ return sOSApp_Win; }

ZOSApp_Win::ZOSApp_Win(const char* inSignature)
	{
	ZAssertStop(kDebug_Win, sOSApp_Win == nil);
	sOSApp_Win = this;
	ZDCScratch::sSet(new ZDCCanvas_GDI_IC);

	if (inSignature)
		fSignature = inSignature;

	if (fSignature.empty())
		fSignature = "org.zoolib.ZOSApp_Win";

	(new ZThreadSimple<ZOSApp_Win*>(sMessagePump, this, "ZOSApp_Win::sMessagePump"))->Start();

	// Wait till the message pump thread has got started, because some calls that are made
	// against the ZOSApp_Win require that the utility window be already created.
	fSemaphore_MessagePump.Wait(1);
	}

ZOSApp_Win::~ZOSApp_Win()
	{
	ZAssertLocked(1, fMutex_MessageDispatch);

	// We're being destroyed, kill all remaining windows, with extreme prejudice.
	ZMutexLocker structureLocker(fMutex_Structure, false);
	ZAssertStop(kDebug_Win, structureLocker.IsOkay());
	ZMessage theDieMessage;
	theDieMessage.SetString("what", "zoolib:DieDieDie");

	while (true)
		{
		bool hitAnyTopLevel = false;
		for (HWND currentHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
			currentHWND != nil && !hitAnyTopLevel;
			currentHWND = ::GetWindow(currentHWND, GW_HWNDNEXT))
			{
			if (ZOSWindow_Win* currentOSWindow = ZOSWindow_Win::sFromHWNDNilOkay(currentHWND))
				{
				hitAnyTopLevel = true;
				if (ZOSWindow_Frame* currentFrameWindow
					= dynamic_cast<ZOSWindow_Frame*>(currentOSWindow))
					{
					HWND clientHWND = currentFrameWindow->GetClientHWND();
					while (true)
						{
						bool hitAnyMDI = false;
						for (HWND currentMDIHWND = ::GetWindow(clientHWND, GW_CHILD);
							currentMDIHWND != nil && !hitAnyMDI;
							currentMDIHWND = ::GetWindow(currentMDIHWND, GW_HWNDNEXT))
							{
							if (ZOSWindow_Win* currentMDIWindow
								= ZOSWindow_Win::sFromHWNDNilOkay(currentMDIHWND))
								{
								hitAnyMDI = true;
								currentMDIWindow->QueueMessage(theDieMessage);
								fCondition_Structure.Wait(fMutex_Structure);
								}
							}
						if (!hitAnyMDI)
							break;
						}
					}
				currentOSWindow->QueueMessage(theDieMessage);
				fCondition_Structure.Wait(fMutex_Structure);
				}
			}
		if (!hitAnyTopLevel)
			break;
		}

	// Tell our message pump to exit
	::sPostMessage(fHWND_Utility, WM_QUIT, 0, 0);

	// Release the structure lock, against which our message pump thread is probably blocked.
	structureLocker.Release();

	// Wait till the message pump thread has finished
	fSemaphore_MessagePump.Wait(1);

	ZAssertStop(kDebug_Win, fHWND_Utility == nil);

	ZDCScratch::sSet(ZRef<ZDCCanvas>());

	ZAssertStop(kDebug_Win, sOSApp_Win == this);
	sOSApp_Win = nil;
	}

void ZOSApp_Win::Run()
	{
	// On other platforms we wait for a system-generated event before we send RunStarted to our
	// owner, there's no equivalent on windows, so we dispatch it manually here.
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:RunStarted");
	this->DispatchMessageToOwner(theMessage);

	ZOSApp_Std::Run();
	}

string ZOSApp_Win::GetAppName()
	{
	if (ZUtil_Win::sUseWAPI())
		{
		wchar_t buffer[1024];
		DWORD len = ::GetModuleFileNameW(::GetModuleHandleW(nil), buffer, countof(buffer));
		if (len <= 0)
			return "AppName";
		string thePath = ZUnicode::sAsUTF8(buffer);
		size_t offset = thePath.rfind('\\');
		if (offset != string::npos)
			thePath = thePath.substr(offset + 1, string::npos);
		return thePath;
		}
	else
		{
		char buffer[1024];
		DWORD len = ::GetModuleFileNameA(::GetModuleHandleA(nil), buffer, countof(buffer));
		if (len <= 0)
			return "AppName";
		string thePath = buffer;
		size_t offset = thePath.rfind('\\');
		if (offset != string::npos)
			thePath = thePath.substr(offset + 1, string::npos);
		return thePath;
		}
	}

bool ZOSApp_Win::HasUserAccessibleWindows()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (HWND iterHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
		iterHWND != nil; iterHWND = ::GetWindow(iterHWND, GW_HWNDNEXT))
		{
		if (ZOSWindow_Win* currentWindow = ZOSWindow_Win::sFromHWNDNilOkay(iterHWND))
			{
			if (::IsWindowVisible(iterHWND))
				return true;
			}
		}
	return false;
	}

bool ZOSApp_Win::HasGlobalMenuBar()
	{ return false; }

void ZOSApp_Win::BroadcastMessageToAllWindows(const ZMessage& inMessage)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (HWND iterHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
		iterHWND != nil; iterHWND = ::GetWindow(iterHWND, GW_HWNDNEXT))
		{
		if (ZOSWindow_Win* currentWindow = ZOSWindow_Win::sFromHWNDNilOkay(iterHWND))
			{
			currentWindow->QueueMessage(inMessage);
			// If the window is a frame window, then walk through all its children
			if (ZOSWindow_Frame* currentFrameWindow = dynamic_cast<ZOSWindow_Frame*>(currentWindow))
				{
				HWND clientHWND = currentFrameWindow->GetClientHWND();
				for (HWND mdiIterHWND = ::GetWindow(clientHWND, GW_CHILD);
					mdiIterHWND != nil; mdiIterHWND = ::GetWindow(mdiIterHWND, GW_HWNDNEXT))
					{
					// For each child do pretty much the same as what we do for top level windows.
					if (ZOSWindow_Win* currentMDIWindow
						= ZOSWindow_Win::sFromHWNDNilOkay(mdiIterHWND))
						{
						ZAssertStop(kDebug_Win, dynamic_cast<ZOSWindow_MDI*>(currentMDIWindow));
						currentMDIWindow->QueueMessage(inMessage);
						}
					}
				}
			}
		}
	this->QueueMessage(inMessage);
	}

ZOSWindow* ZOSApp_Win::CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes)
	{
	return new ZOSWindow_SDI(inAttributes);
	}

ZOSWindow_Frame* ZOSApp_Win::CreateOSWindow_Frame(const ZOSWindow::CreationAttributes& inAttributes)
	{
	return new ZOSWindow_Frame(inAttributes);
	}

bool ZOSApp_Win::Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
	const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
	bigtime_t inTimeout, ZOSWindow_Std*& outWindow)
	{
	outWindow = nil;
	bigtime_t endTime = ZTicks::sNow() + inTimeout;
	for (HWND iterHWND = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
		iterHWND != nil; iterHWND = ::GetWindow(iterHWND, GW_HWNDNEXT))
		{
		if (ZOSWindow_Win* currentWindow = ZOSWindow_Win::sFromHWNDNilOkay(iterHWND))
			{
			// If the window is a frame window, then walk through all its children
			if (ZOSWindow_Frame* currentFrameWindow = dynamic_cast<ZOSWindow_Frame*>(currentWindow))
				{
				HWND clientHWND = currentFrameWindow->GetClientHWND();
				for (HWND mdiIterHWND = ::GetWindow(clientHWND, GW_CHILD);
					mdiIterHWND != nil; mdiIterHWND = ::GetWindow(mdiIterHWND, GW_HWNDNEXT))
					{
					// For each child do pretty much the same as what we do for top level windows.
					if (ZOSWindow_Win* currentMDIWindow
						= ZOSWindow_Win::sFromHWNDNilOkay(mdiIterHWND))
						{
						ZAssertStop(kDebug_Win, dynamic_cast<ZOSWindow_MDI*>(currentMDIWindow));

						if (!ZUtil_STL::sContains(inVector_VisitedWindows, currentMDIWindow))
							{
							if (!ZUtil_STL::sContains(inVector_DroppedWindows, currentMDIWindow))
								{
								ZThread::Error err
									= currentMDIWindow->GetLock().Acquire(endTime - ZTicks::sNow());

								if (err == ZThread::errorTimeout)
									return false;
								if (err == ZThread::errorNone)
									{
									outWindow = currentWindow;
									return true;
									}
								}
							}
						}
					}
				}
			// If we've got to here, the window could still be a frame window, but we've already
			// handled all its children in some fashion, so we must now try asking the window
			// itself. The window could also be just a normal window of some kind.
			if (!ZUtil_STL::sContains(inVector_VisitedWindows, currentWindow))
				{
				if (!ZUtil_STL::sContains(inVector_DroppedWindows, currentWindow))
					{
					// Not dropped and not visited, so try to lock it and return.
					ZThread::Error err = currentWindow->GetLock().Acquire(endTime - ZTicks::sNow());
					if (err == ZThread::errorTimeout)
						return false;
					if (err == ZThread::errorNone)
						{
						outWindow = currentWindow;
						return true;
						}
					}
				}
			}
		}
	return true;
	}

ZOSWindow_Win* ZOSApp_Win::GetActiveOSWindow()
	{
	if (HWND theActiveHWND = reinterpret_cast<HWND>
		(::sSendMessage(fHWND_Utility, sMSG_GetActive, 0, 0)))
		{
		if (ZOSWindow_Win* theOSWindow = ZOSWindow_Win::sFromHWNDNilOkay(theActiveHWND))
			{
			if (ZOSWindow_Frame* theOSWindow_Frame = dynamic_cast<ZOSWindow_Frame*>(theOSWindow))
				{
				if (ZOSWindow_MDI* theOSWindow_MDI = theOSWindow_Frame->GetActiveOSWindow_MDI())
					return theOSWindow_MDI;
				}
			return theOSWindow;
			}
		}
	return nil;
	}

void ZOSApp_Win::AddOSWindow(ZOSWindow_Win* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	fCondition_Structure.Broadcast();
	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_Win::RemoveOSWindow(ZOSWindow_Win* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	this->Internal_OSWindowRemoved(inOSWindow);

	fCondition_Structure.Broadcast();

	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

ZDragInitiator* ZOSApp_Win::CreateDragInitiator()
	{
	return new DragInitiator(fHWND_Utility);
	}

void* ZOSApp_Win::InvokeFunctionFromMessagePump(MessagePumpProc inMessagePumpProc, void* inParam)
	{
	if (ZThread::sCurrentID() == fThreadID_MessagePump)
		return inMessagePumpProc(inParam);

	return reinterpret_cast<void*>
			(::sSendMessage(fHWND_Utility, sMSG_InvokeFunction,
			reinterpret_cast<WPARAM>(inParam), reinterpret_cast<LPARAM>(inMessagePumpProc)));
	}

void ZOSApp_Win::InvokeFunctionFromMessagePumpNoWait(
	MessagePumpProc inMessagePumpProc, void* inParam)
	{
	if (ZThread::sCurrentID() == fThreadID_MessagePump)
		inMessagePumpProc(inParam);

	::sPostMessage(fHWND_Utility, sMSG_InvokeFunction,
		reinterpret_cast<WPARAM>(inParam), reinterpret_cast<LPARAM>(inMessagePumpProc));
	}

static const UTF16 sWindowProc_Utility_ClassNameW[] = L"ZOSApp_Win::sWindowProc_UtilityW";

void ZOSApp_Win::MessagePumpW()
	{
	HRESULT theHRESULT = ::OleInitialize(nil);
	if (FAILED(theHRESULT))
		ZDebugStopf(1, ("Could not initialize OLE"));

	// Register the task bar dummy window class, used as a parent by toplevel windows that do not
	// want to show up on the task bar (see Paul DiLascia's column in MSJ, issue ???)
	::sRegisterWindowClassW(sWNDCLASSName_TaskBarDummyW, ::DefWindowProcW);

	// Register and create the utility window.
	::sRegisterWindowClassW(sWindowProc_Utility_ClassNameW, sWindowProc_Utility);

	const string16 signature = ZUnicode::sAsUTF16(fSignature);

	fHWND_Utility = ::CreateWindowExW(
		WS_EX_TOOLWINDOW, // Extended attributes
		sWindowProc_Utility_ClassNameW, // window class name
		signature.c_str(), // window caption
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		nil,
		(HMENU)nil,
		::GetModuleHandleW(nil), // program instance handle
		nil); // creation parameters

	UINT theTimerID = ::SetTimer(fHWND_Utility, 1, 50, nil);

	// Record our ID
	fThreadID_MessagePump = ZThread::sCurrentID();

	// Notify that we have started the message pump
	fSemaphore_MessagePump.Signal(1);

	while (true)
		{
		MSG theMSG;
		if (::GetMessageW(&theMSG, nil, 0, 0) == 0)
			break;

		::DispatchMessageW(&theMSG);
		}

	::KillTimer(fHWND_Utility, theTimerID);

	::DestroyWindow(fHWND_Utility);

	fHWND_Utility = nil;

	::OleUninitialize();

	fThreadID_MessagePump = 0;

	// Notify that we have finished the message pump
	fSemaphore_MessagePump.Signal(1);
	}

static const UTF8 sWindowProc_Utility_ClassNameA[] = "ZOSApp_Win::sWindowProc_UtilityA";

void ZOSApp_Win::MessagePumpA()
	{
	HRESULT theHRESULT = ::OleInitialize(nil);
	if (FAILED(theHRESULT))
		ZDebugStopf(1, ("Could not initialize OLE"));

	// Register the task bar dummy window class, used as a parent by toplevel windows that do not
	// want to show up on the task bar (see Paul DiLascia's column in MSJ, issue ???)
	::sRegisterWindowClassA(sWNDCLASSName_TaskBarDummyA, ::DefWindowProcA);

	// Register and create the utility window.
	::sRegisterWindowClassA(sWindowProc_Utility_ClassNameA, sWindowProc_Utility);

	fHWND_Utility = ::CreateWindowExA(
		WS_EX_TOOLWINDOW, // Extended attributes
		sWindowProc_Utility_ClassNameA, // window class name
		fSignature.c_str(), // window caption
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		nil,
		(HMENU)nil,
		::GetModuleHandleA(nil), // program instance handle
		nil); // creation parameters

	UINT theTimerID = ::SetTimer(fHWND_Utility, 1, 50, nil);

	// Record our ID
	fThreadID_MessagePump = ZThread::sCurrentID();

	// Notify that we have started the message pump
	fSemaphore_MessagePump.Signal(1);

	while (true)
		{
		MSG theMSG;
		if (::GetMessageA(&theMSG, nil, 0, 0) == 0)
			break;

		::DispatchMessageA(&theMSG);
		}

	::KillTimer(fHWND_Utility, theTimerID);

	::DestroyWindow(fHWND_Utility);

	fHWND_Utility = nil;

	::OleUninitialize();

	fThreadID_MessagePump = 0;

	// Notify that we have finished the message pump
	fSemaphore_MessagePump.Signal(1);
	}

void ZOSApp_Win::sMessagePump(ZOSApp_Win* inOSApp_Win)
	{
	if (ZUtil_Win::sUseWAPI())
		inOSApp_Win->MessagePumpW();
	else
		inOSApp_Win->MessagePumpA();
	}

LRESULT ZOSApp_Win::WindowProc_Utility(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{
	if (inMessage == sMSG_InvokeFunction)
		{
		LRESULT result = 0;
		try
			{
			result = reinterpret_cast<LRESULT>
				(reinterpret_cast<MessagePumpProc>(inLPARAM)(reinterpret_cast<void*>(inWPARAM)));
			}
		catch (...)
			{}
		return result;
		}
	else if (inMessage == sMSG_InitiateDrag)
		{
		ZOSWindow_Win_DragObject* theDragObject = nil;
		ZDragClip_Win_DropSource* theDropSource = nil;

		DragInitiateInfo* theDragInitiateInfo = reinterpret_cast<DragInitiateInfo*>(inLPARAM);
		try
			{
			theDragObject = new ZOSWindow_Win_DragObject(theDragInitiateInfo->fTuple,
														theDragInitiateInfo->fFeedbackCombo,
														theDragInitiateInfo->fDragSource);
			theDragObject->AddRef();

			theDropSource = new ZDragClip_Win_DropSource;
			theDropSource->AddRef();

			DWORD theEffect;
			HRESULT result = ::DoDragDrop(theDragObject, theDropSource,
				DROPEFFECT_COPY | DROPEFFECT_MOVE, &theEffect);
			}
		catch (...)
			{}

		if (theDragObject)
			theDragObject->Release();
		if (theDropSource)
			theDropSource->Release();

		delete theDragInitiateInfo;
		return 0;
		}
	else if (inMessage == sMSG_GetActive)
		{
		return reinterpret_cast<LRESULT>(::GetActiveWindow());
		}

	switch (inMessage)
		{
		case WM_ACTIVATEAPP:
			{
			break;
			}
		case WM_MENUCHAR:
			{
			return ZMenu_Win::sHandle_MENUCHAR(LOWORD(inWPARAM), HIWORD(inWPARAM),
				reinterpret_cast<HMENU>(inLPARAM));
			}
		case WM_MEASUREITEM:
			{
			MEASUREITEMSTRUCT* theMIS = reinterpret_cast<MEASUREITEMSTRUCT*>(inLPARAM);
			if (theMIS->CtlType == ODT_MENU)
				{
				ZMenu_Win::sHandle_MEASUREITEM(theMIS);
				return 1;
				}
			break;
			}
		case WM_DRAWITEM:
			{
			DRAWITEMSTRUCT* theDIS = reinterpret_cast<DRAWITEMSTRUCT*>(inLPARAM);
			if (theDIS->CtlType == ODT_MENU)
				{
				ZMenu_Win::sHandle_DRAWITEM(theDIS);
				return 1;
				}
			break;
			}
		case WM_TIMER:
			{
			this->Internal_RecordIdle();
			break;
			}
		case WM_QUERYENDSESSION:
			{
			break;
			}
		default:
			{
			break;
			}
		}

	if (ZUtil_Win::sUseWAPI())
		return ::DefWindowProcW(inHWND, inMessage, inWPARAM, inLPARAM);
	else
		return ::DefWindowProcA(inHWND, inMessage, inWPARAM, inLPARAM);
	}

LRESULT CALLBACK ZOSApp_Win::sWindowProc_Utility(HWND inHWND, UINT inMessage,
	WPARAM inWPARAM, LPARAM inLPARAM)
	{ return sOSApp_Win->WindowProc_Utility(inHWND, inMessage, inWPARAM, inLPARAM); }

HWND ZOSApp_Win::GetHWND_Utility()
	{ return fHWND_Utility; }

// =================================================================================================

#endif // ZCONFIG(API_OSWindow, Win32)

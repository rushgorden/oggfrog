/*  @(#) $Id: ZOSWindow_Win.h,v 1.7 2006/04/14 21:02:35 agreen Exp $ */

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

#ifndef __ZOSWindow_Win__
#define __ZOSWindow_Win__ 1
#include "zconfig.h"

#if ZCONFIG(API_OSWindow, Win32)

#include "ZDragClip.h"
#include "ZOSWindow_Std.h"
#include "ZMenu.h"
#include "ZWinHeader.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win_DragFeedbackCombo

class ZOSWindow_Win_DragFeedbackCombo
	{
public:
	ZPoint fHotPoint;
	ZDCRgn fRgn;
	ZDCPixmap fPixmap;
	ZDCRgn fMaskRgn;
	ZDCPixmap fMaskPixmap;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win

class ZOSApp_Win;
class ZDCCanvas_GDI_Window;

class ZOSWindow_Win : public ZOSWindow_Std
	{
public:
	ZOSWindow_Win(bool inHasSizeBox, bool inHasResizeBorder, bool inHasCloseBox,
		bool inHasMenuBar, int32 inZLevel);
	virtual ~ZOSWindow_Win();

// From ZOSWindow via ZOSWindow_Std
	virtual bool DispatchMessage(const ZMessage& inMessage);

	virtual void SetShownState(ZShownState inState);

	virtual void SetTitle(const string& inTitle);
	virtual string GetTitle();

	virtual void SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap,
		const ZDCPixmap& inMaskPixmap);
	virtual ZCoord GetTitleIconPreferredHeight();

	virtual void SetCursor(const ZCursor& inCursor);

	virtual void SetLocation(ZPoint inLocation);

	virtual void SetSize(ZPoint inSize);

	virtual void SetFrame(const ZRect& inFrame);

	virtual void SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize);

	virtual void BringFront(bool inMakeVisible);

	virtual void GetContentInsets(ZPoint& topLeftInset, ZPoint& bottomRightInset);

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();

	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void GetNative(void* outNative);

	virtual ZDragInitiator* CreateDragInitiator();

	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly);

// From ZOSWindow_Std
	virtual void Internal_Update(const ZDCRgn& inUpdateRgn);
	virtual ZDCRgn Internal_GetExcludeRgn();
	virtual void Internal_TitleIconClick(const ZMessage& inMessage);

// Our protocol
	void FinalizeCanvas(ZDCCanvas_GDI_Window* inCanvas);

	static ZOSWindow_Win* sFromHWNDNilOkayW(HWND inHWND);
	static ZOSWindow_Win* sFromHWNDNilOkayA(HWND inHWND);
	static ZOSWindow_Win* sFromHWNDNilOkay(HWND inHWND);

	static ZOSWindow_Win* sFromHWNDW(HWND inHWND);
	static ZOSWindow_Win* sFromHWNDA(HWND inHWND);

	HWND GetHWND() { return fHWND; }

	int32 Internal_GetZLevel() { return fZLevel; }

protected:
	void Internal_RecordDrag(bool inIsContained, IDataObject* inDataObject, DWORD inKeyState,
		POINTL inLocation, DWORD* outEffect);
	void Internal_RecordDrop(IDataObject* inDataObject, DWORD inKeyState,
		POINTL inLocation, DWORD* outEffect);
	void Internal_UpdateDragFeedback();

// Utility methods
	static void sCalcContentInsets(HWND inHWND, ZPoint& outTopLeftInset, ZPoint& inBottomRightInset);

	static void sFromCreationAttributes(const CreationAttributes& inAttributes,
		DWORD& outWindowStyle, DWORD& outExWindowStyle);

	virtual LRESULT CallBaseWindowProc(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM) = 0;

	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProcW(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProcA(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProc_MDIW(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProc_MDIA(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

protected:
	HWND fHWND;

	HPALETTE fHPALETTE;

	bool fCheckMouseOnTimer;
	bool fContainedMouse;
	bool fRequestedCapture;
	ZCursor fCursor;

	bool fHasSizeBox;
	bool fHasResizeBorder;
	bool fHasMenuBar;
	bool fHasCloseBox;
	int32 fZLevel;

	ZRect fSizeBoxFrame;

	ZDCCanvas_GDI_Window* fCanvas;
	friend class ZDCCanvas_GDI_Window;

	ZPoint fSize_Min;
	ZPoint fSize_Max;

	ZOSWindow_Win_DragFeedbackCombo fDrag_RecordedFeedback;
	ZOSWindow_Win_DragFeedbackCombo fDrag_ReportedFeedback;

	ZPoint fDrag_RecordedLocation;
	ZPoint fDrag_ReportedLocation;
	bool fDrag_RecordedContained;
	bool fDrag_ReportedContained;
	ZDragSource* fDrag_DragSource;
	ZTuple fDrag_Tuple;

	bool fDrag_PostedMessage;

	class DropTarget;
	friend class DropTarget;
	DropTarget* fDropTarget;
	class DragDropImpl;
	class Drag;
	class Drop;

	class WindowProcFilter;
	friend class WindowProcFilter;
	WindowProcFilter* fWindowProcFilter_Head;

	friend class ZOSApp_Win;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Win::WindowProcFilter

class ZOSWindow_Win::WindowProcFilter
	{
public:
	WindowProcFilter(ZOSWindow_Win* inWindow);
	virtual ~WindowProcFilter();
	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

protected:
	WindowProcFilter* fPrev;
	WindowProcFilter* fNext;
	ZOSWindow_Win* fWindow;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_TopLevel

class ZOSWindow_TopLevel : public ZOSWindow_Win
	{
public:
	ZOSWindow_TopLevel(bool inHasSizeBox, bool inHasResizeBorder, bool inHasCloseBox,
		bool inHasMenuBar, ZOSWindow::Layer inLayer);
	virtual ~ZOSWindow_TopLevel();

// From ZOSWindow_Win
	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

// Our protocol
	bool Internal_CanMoveWithinLevel() { return fCanMoveWithinLevel; }
	bool Internal_CanBeTargetted() { return fCanBeTargetted; }
	bool Internal_IsModal() { return fIsModal; }


protected:
	HWND fHWND_TaskBarDummy;

	bool fCanMoveWithinLevel;
	bool fCanBeTargetted;
	bool fIsModal;
	bool fAppearsOnTaskBar;

	bool fHideOnSuspend;
	bool fVisibleBeforeSuspend;
	bool fSuspended;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_SDI

class ZOSWindow_SDI : public ZOSWindow_TopLevel
	{
protected:
	static void* sCreate(void* inParam);
	void CreateW(const ZOSWindow::CreationAttributes& inAttributes);
	void CreateA(const ZOSWindow::CreationAttributes& inAttributes);

public:
	ZOSWindow_SDI(const ZOSWindow::CreationAttributes& inAttributes);
	virtual ~ZOSWindow_SDI();

// From ZOSWindow via ZOSWindow_TopLevel
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

// From ZOSWindow_Win via ZOSWindow_TopLevel
	virtual LRESULT CallBaseWindowProc(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

protected:
	ZMenuBar fMenuBar;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Frame

class ZOSWindow_MDI;
class ZOSWindow_Frame : public ZOSWindow_TopLevel
	{
protected:
	static void* sCreate(void* inParam);
	void CreateW(const ZOSWindow::CreationAttributes& inAttributes);
	void CreateA(const ZOSWindow::CreationAttributes& inAttributes);

public:
	ZOSWindow_Frame(const ZOSWindow::CreationAttributes& inAttributes);
	virtual ~ZOSWindow_Frame();

// From ZOSWindow via ZOSWindow_TopLevel
	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

// From ZOSWindow_Win via ZOSWindow_TopLevel
	virtual LRESULT CallBaseWindowProc(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

// Our protocol
	ZOSWindow_MDI* CreateOSWindow_MDI(const ZOSWindow::CreationAttributes& inAttributes);
	ZOSWindow_MDI* GetActiveOSWindow_MDI();

	LRESULT WindowProc_ClientW(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);
	LRESULT WindowProc_ClientA(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProc_ClientW(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	static LRESULT CALLBACK sWindowProc_ClientA(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	HWND GetClientHWND() { return fHWND_Client; }

	void SetClientInsets(ZPoint inClientInsetTopLeft, ZPoint inClientInsetBottomRight);
	void GetClientInsets(ZPoint& outClientInsetTopLeft, ZPoint& outClientInsetBottomRight);


protected:
	HWND fHWND_Client;
	WNDPROC fWNDPROC_Client;
	ZPoint fClientInsetTopLeft;
	ZPoint fClientInsetBottomRight;

//	ZMenuBar fMenuBar;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_MDI

class ZOSWindow_MDI : public ZOSWindow_Win
	{
protected:
	static void* sCreate(void* inParam);
	void CreateW(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes);
	void CreateA(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes);

public:
	ZOSWindow_MDI(HWND inHWND_Client, const ZOSWindow::CreationAttributes& inAttributes);
	virtual ~ZOSWindow_MDI();

// From ZOSWindow via ZOSWindow_Win
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void Center();
	virtual void CenterDialog();
	virtual ZPoint ToGlobal(const ZPoint& inWindowPoint);
	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

// From ZOSWindow_Win
	virtual LRESULT CallBaseWindowProc(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	virtual LRESULT WindowProc(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);

// Our protocol
	void Center(bool inHorizontal, bool inVertical, bool inForDialog);
	HMENU GetHMENU();
	ZOSWindow_Frame* GetOSWindow_Frame();

protected:
	ZMenuBar fMenuBar;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Win

class ZOSApp_Win : public ZOSApp_Std
	{
public:
	static ZOSApp_Win* sGet();

	ZOSApp_Win(const char* inSignature);
	virtual ~ZOSApp_Win();

// From ZOSApp via ZOSApp_Std
	virtual void Run();

	virtual string GetAppName();

	virtual bool HasUserAccessibleWindows();

	virtual bool HasGlobalMenuBar();

	virtual void BroadcastMessageToAllWindows(const ZMessage& inMessage);
	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes);
	virtual ZOSWindow_Frame* CreateOSWindow_Frame(const ZOSWindow::CreationAttributes& inAttributes);

// From ZOSApp_Std
	virtual bool Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
		const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
		bigtime_t inTimeout, ZOSWindow_Std*& outWindow);

// Our protocol
	ZOSWindow_Win* GetActiveOSWindow();

	void AddOSWindow(ZOSWindow_Win* inOSWindow);
	void RemoveOSWindow(ZOSWindow_Win* inOSWindow);

	ZDragInitiator* CreateDragInitiator();

	typedef void* (*MessagePumpProc)(void* inParam);
	void* InvokeFunctionFromMessagePump(MessagePumpProc inMessagePumpProc, void* inParam);
	void InvokeFunctionFromMessagePumpNoWait(MessagePumpProc inMessagePumpProc, void* inParam);

	void MessagePumpW();
	void MessagePumpA();
	static void sMessagePump(ZOSApp_Win* inOSApp_Win);

	LRESULT WindowProc_Utility(HWND inHWND, UINT inMessage, WPARAM inWPARAM, LPARAM inLPARAM);
	static LRESULT CALLBACK sWindowProc_Utility(HWND inHWND, UINT inMessage,
		WPARAM inWPARAM, LPARAM inLPARAM);

	HWND GetHWND_Utility();

protected:
	static ZOSApp_Win* sOSApp_Win;

	HWND fHWND_Utility;
	ZThread::ThreadID fThreadID_MessagePump;
	ZSemaphore fSemaphore_MessagePump;

	string fSignature;
	
	class DragInitiator;
	};

// =================================================================================================

#endif // ZCONFIG(API_OSWindow, Win32)

#endif // __ZOSWindow_Win__

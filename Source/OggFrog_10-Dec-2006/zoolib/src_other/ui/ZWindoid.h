/*  @(#) $Id: ZWindoid.h,v 1.6 2006/04/10 20:44:23 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZWindoid__
#define __ZWindoid__ 1
#include "zconfig.h"

#include "ZOSWindow_Std.h"
#include "ZPane.h"
#include "ZDragClip.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoidPane

class ZWindoid;
class ZWindoidPaneCaptureInput;

class ZWindoidPane : public ZSuperPane, public ZMessageReceiver
	{
public:
	ZWindoidPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZMessageLooper* inLooper);
	virtual ~ZWindoidPane();

// From ZSubPane
	virtual void Activate(bool entering);

	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);

	virtual void DoIdle();

	virtual void HandleFramePreChange(ZPoint inWindowLocation);
	virtual ZDCRgn HandleFramePostChange(ZPoint inWindowLocation);

	virtual void ScrollPreChange();
	virtual void ScrollPostChange();

// From ZMessageReceiver
	virtual void ReceivedMessage(const ZMessage& iMessage);

// Our protocol
	void InvalidateWindoidWindowRgn(ZWindoid* inWindoid, const ZDCRgn& inRgn);
	void InvalidateWindoidRgn(ZWindoid* inWindoid, const ZDCRgn& inRgn);
	ZDCRgn GetWindoidVisibleContentRgn(ZWindoid* inWindoid);
	ZDCRgn CalcAboveWindoidRgn(ZWindoid* inWindoid);

	void SetWindoidAttributes(ZWindoid* inWindoid, ZWindoid* inBehindWindoid, ZPoint inLocation, const ZDCRgn& inStructureRgn, const ZDCRgn& inContentRgn, bool inShown);

	ZPoint ToWindoid(ZWindoid* inWindoid, ZPoint inPanePoint);
	ZPoint FromWindoid(ZWindoid* inWindoid, ZPoint inWindoidPoint);

protected:
	void Internal_AddWindoid(ZWindoid* inWindoid);
	void Internal_RemoveWindoid(ZWindoid* inWindoid);

	void Internal_FixupActive();

	ZWindoid* Internal_FindWindoid(ZPoint inPanePoint);

	void QueueMessageForWindoid(ZWindoid* inWindoid, const ZMessage& inMessage);

	void AcquireCapture(ZWindoid* inWindoid);
	void ReleaseCapture(ZWindoid* inWindoid);
	void HandleCaptureMessage(const ZMessage& inMessage);

	ZWindoid* fWindoid_Top;
	ZWindoid* fWindoid_Bottom;

	ZWindoid* fWindoid_Capture;
	ZWindoidPaneCaptureInput* fCaptureInput;

	ZPoint fOrigin;

	friend class ZWindoid;
	friend class ZWindoidPaneCaptureInput;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoid

class ZWindoid : public ZOSWindow
	{
private:
	ZWindoid(const ZWindoid&); // Not implemented
	ZWindoid& operator=(ZWindoid&); // Not implemented

public:
	ZWindoid(ZWindoidPane* inWindoidPane);
	virtual ~ZWindoid();

	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& inMessage);

	virtual void SetShownState(ZShownState inState);
	virtual ZShownState GetShownState();

	virtual void SetTitle(const string& inTitle);
	virtual string GetTitle();

	virtual void SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap);
	virtual ZCoord GetTitleIconPreferredHeight();

	virtual void SetCursor(const ZCursor& inCursor);

	virtual void SetLocation(ZPoint inLocation);
	virtual ZPoint GetLocation();

	virtual void PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists);
	virtual void EndPoseModal();
	virtual bool WaitingForPoseModal();

	virtual void BringFront(bool inMakeVisible);

	virtual bool GetActive();

	virtual void GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual ZPoint GetOrigin();

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual ZPoint ToGlobal(const ZPoint& inWindowPoint);

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();

	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void GetNative(void* outNative);

	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

// Our protocol
	virtual void DrawContent(const ZDC& inDC) = 0;
	virtual void DrawStructure(const ZDC& inDC, bool inActive, bool inPaintBackground) = 0;
	virtual string FindHitPart(ZPoint inWindoidPoint);
	virtual void HandleClick(ZPoint inWindoidPoint, const string& inHitPart, const ZMessage& inMessage);
	virtual ZPoint ConstrainSize(ZPoint inSize) = 0;

	void SetRegions(const ZDCRgn& inStructure, const ZDCRgn& inContent);
	void SetRegions(ZPoint inLocation, const ZDCRgn& inStructure, const ZDCRgn& inContent);

	ZDCRgn GetStructureRgn() { return fStructureRgn; }

	void Internal_HandleClick(const ZMessage& inMessage);

protected:
	ZWindoidPane* fWindoidPane;

private:
	bool fShown;
	bool fActive;
	ZPoint fLocation;
	int32 fLayer;

	ZDCRgn fStructureRgn;
	ZDCRgn fContentRgn;

	ZWindoid* fWindoid_Above;
	ZWindoid* fWindoid_Below;

	friend class ZWindoidPane;
	friend class ZWindoidResizeTracker;
	friend class ZWindoidDragTracker;
	};

// =================================================================================================

#endif // __ZWindoid__

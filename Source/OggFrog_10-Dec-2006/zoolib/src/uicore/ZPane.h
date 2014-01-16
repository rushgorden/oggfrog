/*  @(#) $Id: ZPane.h,v 1.5 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZPane__
#define __ZPane__ 1
#include "zconfig.h"

#include "ZCursor.h"
#include "ZDC.h"
#include "ZEventHr.h"
#include "ZRefCount.h"
#include "ZTypes.h"

class ZDragDropHandler;
class ZSubPane;
class ZSuperPane;

// ==================================================

typedef short EZTriState;
enum
	{
	eZTriState_Off, eZTriState_Normal = eZTriState_Off,
	eZTriState_Mixed, eZTriState_Other = eZTriState_Mixed,
	eZTriState_On, eZTriState_Hilited = eZTriState_On
	};

// ==================================================

typedef short EZAlignment;
enum
	{
	eZAlignment_Left = 0, eZAlignment_Top = eZAlignment_Left,
	eZAlignment_Center = 1,
	eZAlignment_Right = -1, eZAlignment_Bottom = eZAlignment_Right,
	eZAlignment_HotSpot = 2
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneAdorner

class ZPaneAdorner : public ZRefCounted
	{
public:
	ZPaneAdorner();
	virtual ~ZPaneAdorner();

	virtual void AdornPane(const ZDC& inDC, ZSubPane* inPane, const ZDCRgn& inPaneBoundsRgn);
	virtual void Refresh(ZSubPane* inPane);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocator

class ZPaneLocator
	{
public:
	ZPaneLocator(ZPaneLocator* inNextLocator);
	virtual ~ZPaneLocator();

	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
	virtual bool GetPaneVisible(ZSubPane* inPane, bool& outVisible);
	virtual bool GetPaneEnabled(ZSubPane* inPane, bool& outEnabled);
	virtual bool GetPaneHilite(ZSubPane* inPane, EZTriState& outHilite);
	virtual bool GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk);
	virtual bool GetPaneCursor(ZSubPane* inPane, ZPoint inPoint, ZCursor& outCursor);

	virtual bool GetPaneBringsFront(ZSubPane* inPane, bool& outBringsFront);

	virtual bool GetPaneDragDropHandler(ZSubPane* inPane, ZDragDropHandler*& outDragDropHandler);

	virtual bool GetPaneTranslation(ZSuperPane* inPane, ZPoint& outTranslation);
	virtual bool GetPaneInternalSize(ZSuperPane* inPane, ZPoint& outSize);

	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

	virtual void ReferencingSubPaneAdded(ZSubPane* inSubPane);
	virtual void ReferencingSubPaneRemoved(ZSubPane* inSubPane);

	virtual void ReferencingPaneLocatorAdded(ZPaneLocator* inPaneLocator);
	virtual void ReferencingPaneLocatorRemoved(ZPaneLocator* inPaneLocator);

	void PaneLocatorDeleted(ZPaneLocator* inPaneLocator);

	ZPaneLocator* GetNextPaneLocator()
		{ return fNextPaneLocator; }

protected:
	vector<ZPaneLocator*>* fReferencingPaneLocators;
	vector<ZSubPane*>* fReferencingSubPanes;
	ZPaneLocator* fNextPaneLocator;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorSimple

class ZPaneLocatorSimple : public ZPaneLocator
	{
public:
	ZPaneLocatorSimple(ZPaneLocator* inNextLocator);

	virtual void ReferencingPaneLocatorRemoved(ZPaneLocator* inPaneLocator);
	virtual void ReferencingSubPaneRemoved(ZSubPane* inSubPane);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorFixed

class ZPaneLocatorFixed : public ZPaneLocatorSimple
	{
public:
	ZPaneLocatorFixed(ZPoint inLocation, ZPaneLocator* inNextLocator = nil);
	ZPaneLocatorFixed(ZPoint inLocation, ZPoint inSize, ZPaneLocator* inNextLocator = nil);
	ZPaneLocatorFixed(ZCoord inLeft, ZCoord inTop, ZPaneLocator* inNextLocator = nil);
	ZPaneLocatorFixed(ZCoord inLeft, ZCoord inTop, ZCoord inWidth, ZCoord inHeight, ZPaneLocator* inNextLocator = nil);

	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);

private:
	ZPoint fLocation;
	ZPoint fSize;
	bool fGotSize;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorFixedAligned

class ZPaneLocatorFixedAligned : public ZPaneLocatorSimple
	{
public:
	ZPaneLocatorFixedAligned(ZPoint inPosition, EZAlignment inAlignmentH, ZPaneLocator* inNextLocator = nil);
	ZPaneLocatorFixedAligned(ZPoint inPosition, EZAlignment inAlignmentH, EZAlignment inAlignmentV, ZPaneLocator* inNextLocator = nil);

	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);

private:
	ZPoint fPosition;
	EZAlignment fAlignmentH;
	EZAlignment fAlignmentV;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZSubPane

class ZDrop;
class ZFakeWindow;

class ZSubPane : public ZEventHr
	{
public:
	ZSubPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	ZSubPane(ZEventHr* inNextHandler, ZPaneLocator* inPaneLocator);

	virtual ~ZSubPane();

// From ZEventHr
	virtual bool WantsToBecomeTarget();
	virtual bool WantsToBecomeTabTarget();

// Managing the hierarchy
	ZSuperPane* GetSuperPane()
		{ return fSuperPane; }
	void BeInSuperPane(ZSuperPane* inSuperPane);

// Our other parts of our protocol
	virtual ZSuperPane* GetOutermostSuperPane();
	virtual ZFakeWindow* GetWindow();

	virtual void Activate(bool entering);
	ZMutexBase& GetLock();

// Drawing
	virtual void HandleDraw(const ZDC& inDC);
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	void DrawNow();

// Getting a ZDC to reflect the pane's current state.
	ZDC GetDC();
	ZDC GetDC(ZDCRgn& outBoundsRgn);

// Finding panes by location
	virtual ZSubPane* FindPane(ZPoint inPoint);

// Clicks
	virtual bool HandleClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	virtual bool GetBringsFront();

// Mouse motion
	virtual void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);

// Cursor
	virtual bool GetCursor(ZPoint inPoint, ZCursor& outCursor);
	void InvalidateCursor();

// Drag and Drop
	virtual ZDragDropHandler* GetDragDropHandler();

// Invalidation
	void Invalidate(const ZDCRgn& inRgn);

	void Refresh();
	void Update();

// Pane locator
	ZPaneLocator* GetPaneLocator()
		{ return fPaneLocator; }
	void SetPaneLocator(ZPaneLocator* inPaneLocator, bool inRefresh);
	void PaneLocatorDeleted(ZPaneLocator* inPaneLocator);

// Attributes - geometry
	virtual ZPoint GetLocation();
	virtual ZPoint GetSize();
	virtual ZPoint GetCumulativeTranslation();

// Attributes - visibility
	virtual bool GetVisible();
	bool GetReallyVisible();

// Attributes - active state
	bool GetActive();

// Attributes - enable state
	virtual bool GetEnabled();
	bool GetReallyEnabled();

// Attributes - hilited, background color etc
	virtual EZTriState GetHilite();

	virtual ZDCInk GetBackInk(const ZDC& inDC);
	ZDCInk GetRealBackInk(const ZDC& inDC);

// Attributes - for generic expansion
	virtual bool GetAttribute(const string& inAttribute, void* inParam, void* outResult);

// Targetting
	bool IsWindowTarget();

// Idling
	virtual void DoIdle();

// Boundary stuff - generally derived by calls to GetLocation/GetSize etc.
// Our bounds is how big *we* think we are
	virtual ZDCRgn CalcBoundsRgn();
	ZRect CalcBounds();
// Visible bounds is what can actually be seen by the user
	virtual ZDCRgn CalcVisibleBoundsRgn();
	ZRect CalcVisibleBounds();
// Aperture is what could be seen by the user if our window were not obscured by anthing
	ZDCRgn CalcApertureRgn();
	ZRect CalcAperture();
// Same as our bounds rgn, but in our superPane's coordinate system
	ZDCRgn CalcFrameRgn();
	ZRect CalcFrame();

// Boundary/State change notifications
	virtual void FramePreChange();
	virtual void FramePostChange();
	virtual void HandleFramePreChange(ZPoint inWindowLocation);
	virtual void DoFramePreChange();
	virtual ZDCRgn HandleFramePostChange(ZPoint inWindowLocation);
	virtual ZDCRgn DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds);

	virtual void ScrollPreChange();
	virtual void ScrollPostChange();

// Adorners
	void InstallAdorner(ZRef<ZPaneAdorner> inAdorner);
	void RemoveAdorner(ZRef<ZPaneAdorner> inAdorner);

// For ensuring our selection is visible to the user
	virtual void BringPointIntoViewPinned(ZPoint inPosition);
	virtual void BringPointIntoView(ZPoint inPosition);
	virtual void BringSelectionIntoView();

// Coordinate conversions
	virtual ZPoint ToSuper(const ZPoint& inPoint); // Only virtual member of this group
	ZPoint FromSuper(const ZPoint& inPoint);
	ZRect ToSuper(const ZRect& inRect);
	ZRect FromSuper(const ZRect& inRect);
	ZDCRgn ToSuper(const ZDCRgn& inRgn);
	ZDCRgn FromSuper(const ZDCRgn& inRgn);

	virtual ZPoint ToWindow(const ZPoint& inPoint); // Only virtual member of this group
	ZPoint FromWindow(const ZPoint& inPoint);
	ZRect ToWindow(const ZRect& inRect);
	ZRect FromWindow(const ZRect& inRect);
	ZDCRgn ToWindow(const ZDCRgn& inRgn);
	ZDCRgn FromWindow(const ZDCRgn& inRgn);

	virtual ZPoint ToGlobal(const ZPoint& inPoint); // Only virtual member of this group
	ZPoint FromGlobal(const ZPoint& inPoint);
	ZRect ToGlobal(const ZRect& inRect);
	ZRect FromGlobal(const ZRect& inRect);
	ZDCRgn ToGlobal(const ZDCRgn& inRgn);
	ZDCRgn FromGlobal(const ZDCRgn& inRgn);
protected:
	ZSuperPane* fSuperPane;
	ZPaneLocator* fPaneLocator;
	vector<ZRef<ZPaneAdorner> >* fAdorners;
	friend class ZSuperPane;
private:
// AG 98-06-29. This is *private*. It's only here to allow flicker-free window resizing.
	ZRect fPriorBounds;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZSuperPane

class ZSuperPane : public ZSubPane
	{
public:
	ZSuperPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	ZSuperPane(ZEventHr* inNextHandler, ZPaneLocator* inLocator);
	virtual ~ZSuperPane();

// From ZEventHr
	virtual void InsertTabTargets(vector<ZEventHr*>& inOutTargets);

// From ZSubPane
	virtual ZSuperPane* GetOutermostSuperPane();
	virtual void Activate(bool inEntering);

	virtual void HandleDraw(const ZDC& inDC);
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);

	virtual ZSubPane* FindPane(ZPoint inPoint);

	virtual bool HandleClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	virtual ZPoint GetCumulativeTranslation();

	virtual void HandleFramePreChange(ZPoint inWindowLocation);
	virtual ZDCRgn HandleFramePostChange(ZPoint inWindowLocation);
	virtual ZDCRgn DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds);

	virtual void ScrollPreChange();
	virtual void ScrollPostChange();

// Our Protocol
	virtual ZPoint GetTranslation();
	virtual ZPoint GetInternalSize();
	virtual ZDCInk GetInternalBackInk(const ZDC& inDC);
	virtual void SubPaneAdded(ZSubPane* inPane);
	virtual void SubPaneRemoved(ZSubPane* inPane);
	size_t GetSubPaneCount();
	size_t GetSubPaneIndex(ZSubPane* inPane);
	ZSubPane* GetSubPaneAt(size_t inIndex);

	ZDCRgn GetSubPaneRgn();
protected:
	vector<ZSubPane*> fSubPanes;
	};

// =================================================================================================

#endif // __ZPane__

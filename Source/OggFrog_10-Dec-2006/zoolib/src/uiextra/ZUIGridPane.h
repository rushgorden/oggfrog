/*  @(#) $Id: ZUIGridPane.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUIGridPane__
#define __ZUIGridPane__ 1
#include "zconfig.h"

#include "ZBigRegion.h"
#include "ZMouseTracker.h"
#include "ZUI.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane

class ZUIGridPane : public ZSuperPane, public ZUIScrollable
	{
public:
	class Selection;
	class Geometry;
	class CellRenderer;

	ZUIGridPane(ZSuperPane* superPane, ZPaneLocator* paneLocator, Geometry* inGeometryH, Geometry* inGeometryV, CellRenderer* inCellRenderer, Selection* inSelection);
	virtual ~ZUIGridPane();

// From ZSuperPane/ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);
	virtual ZDCRgn DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds);
	virtual bool WantsToBecomeTarget();

	virtual void BringPointIntoViewPinned(ZPoint inPosition);
	virtual void BringPointIntoView(ZPoint inPosition);

// From ZUIScrollable
	virtual void ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications);

	virtual void ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);

	virtual ZPoint_T<double> GetScrollValues();
	virtual ZPoint_T<double> GetScrollThumbProportions();

// Our protocol
	void BringCellIntoView(ZPoint_T<int32> inCell);

	ZPoint_T<int32> GetMaxCell();
	bool CellInRange(ZPoint_T<int32> inCell);
	ZRect_T<int32> GetVisibleCells(bool inOnlyCompletelyVisible);

	ZPoint_T<int32> FindCell(ZPoint inPoint, bool inClampIt);
	bool GetCellRect(ZPoint_T<int32> inCell, ZRect& outRect);
	bool GetCellsRect(ZRect_T<int32> inCells, ZRect& outRect);
	ZDCRgn GetCellPixels(const ZBigRegion& inCells);

	void InvalidateCell(ZPoint_T<int32> inCell);

	void ScrollBy(ZPoint_T<int32> inDelta, bool inDrawIt, bool inDoNotifications);
	void ScrollTo(ZPoint_T<int32> inNewScrollPos, bool inDrawIt, bool inDoNotifications);
	void Internal_ScrollTo(ZPoint_T<int32> inNewScrollPos, bool inDrawIt, bool inDoNotifications);

	Geometry* GetGeometryH()
		{ return fGeometryH; }
	Geometry* GetGeometryV()
		{ return fGeometryV; }
	CellRenderer* GetCellRenderer()
		{ return fCellRenderer; }
	Selection* GetSelection()
		{ return fSelection; }

	class Selection_Single;
	class Selection_SingleStandard;
	class Selection_Multiple;
	class Selection_MultipleStandard;
	class Geometry_Fixed;
	class Geometry_Variable;

protected:
	ZPoint_T<int32> Internal_CalcBottomCell(ZPoint inVisiblePixels);

	ZPoint_T<int32> fScrollPos;
	ZPoint fCellScrollPos;
	Geometry* fGeometryH;
	Geometry* fGeometryV;
	CellRenderer* fCellRenderer;
	Selection* fSelection;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::CellRenderer

class ZUIGridPane::CellRenderer
	{
public:
	CellRenderer();
	virtual ~CellRenderer();

	virtual void DrawCell(const ZDC& inDC, ZPoint_T<int32> inCell, ZRect inCellBounds, bool inSelected, bool inIsAnchor) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection

class ZUIGridPane::Selection : public ZUISelection
	{
public:
	class Responder;

	Selection();
	virtual ~Selection();

	virtual bool Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent);

	virtual bool GetSelected(ZPoint_T<int32> inCell) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Single

class ZUIGridPane::Selection_Single : public ZUIGridPane::Selection
	{
public:
	Selection_Single();
	virtual ~Selection_Single();

// From ZUIGridPane::Selection
	virtual bool GetSelected(ZPoint_T<int32> inCell);

	virtual bool Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent);

// Our protocol
	virtual ZPoint_T<int32> GetSelectedCell() = 0;
	virtual void SetSelectedCell(ZUIGridPane* inGridPane, ZPoint_T<int32> inCell, bool inNotify) = 0;
	virtual void SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify) = 0;

	virtual bool HasSelectedCell();

protected:
	class Tracker;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Single::Tracker

class ZUIGridPane::Selection_Single::Tracker : public ZMouseTracker
	{
public:
	Tracker(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Single* inSelection, bool inRestoreInitialSelectionIfOutOfBounds);
	virtual ~Tracker();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	Selection_Single* fSelection;
	ZUIGridPane* fGridPane;
	ZPoint_T<int32> fInitialSelectedCell;
	bool fRestoreInitialSelectionIfOutOfBounds;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_SingleStandard

class ZUIGridPane::Selection_SingleStandard : public ZUIGridPane::Selection_Single
	{
public:
	Selection_SingleStandard();
	virtual ~Selection_SingleStandard();

// From ZUIGridPane::Selection_Single
	virtual ZPoint_T<int32> GetSelectedCell();
	virtual void SetSelectedCell(ZUIGridPane* inGridPane, ZPoint_T<int32> inCell, bool inNotify);
	virtual void SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify);

protected:
	ZPoint_T<int32> fSelectedCell;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple

class ZUIGridPane::Selection_Multiple : public ZUIGridPane::Selection
	{
public:
	Selection_Multiple();
	virtual ~Selection_Multiple();

// From ZUIGridPane::Selection
	virtual bool GetSelected(ZPoint_T<int32> inCell);

	virtual bool Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent);

// Our protocol
	virtual ZBigRegion GetSelectedCells() = 0;
	virtual void SetSelectedCells(ZUIGridPane* inGridPane, const ZBigRegion& inNewSelection, bool inNotify) = 0;
	virtual void SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify) = 0;

protected:
	class TrackerSingle;
	class TrackerShift;
	class TrackerCommand;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerSingle

class ZUIGridPane::Selection_Multiple::TrackerSingle : public ZMouseTracker
	{
public:
	TrackerSingle(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection);
	virtual ~TrackerSingle();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	Selection_Multiple* fSelection;
	ZUIGridPane* fGridPane;
	ZBigRegion fPriorSelection;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerShift

class ZUIGridPane::Selection_Multiple::TrackerShift : public ZMouseTracker
	{
public:
	TrackerShift(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection);
	virtual ~TrackerShift();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	Selection_Multiple* fSelection;
	ZUIGridPane* fGridPane;
	ZPoint_T<int32> fAnchorCell;
	ZBigRegion fPriorSelection;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerCommand

class ZUIGridPane::Selection_Multiple::TrackerCommand : public ZMouseTracker
	{
public:
	TrackerCommand(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection);
	virtual ~TrackerCommand();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	Selection_Multiple* fSelection;
	bool fDeselecting;
	ZUIGridPane* fGridPane;
	ZBigRegion fPriorSelection;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_MultipleStandard

class ZUIGridPane::Selection_MultipleStandard : public ZUIGridPane::Selection_Multiple
	{
public:
	Selection_MultipleStandard();
	virtual ~Selection_MultipleStandard();

// From Selection_Multiple
	virtual ZBigRegion GetSelectedCells();
	virtual void SetSelectedCells(ZUIGridPane* inGridPane, const ZBigRegion& inNewSelection, bool inNotify);
	virtual void SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify);

protected:
	ZBigRegion fSelection;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry

class ZUIGridPane::Geometry
	{
public:
	Geometry();
	virtual ~Geometry();

// Stuff that must be overridden
	virtual int32 GetIterationCost(bool inHorizontal) = 0;
	virtual void DrawRangeMaster(bool inHorizontal, const ZDC& inDC,
										ZCoord inBeginCoordMaster, ZCoord inEndCoordMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inBeginCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Geometry* inSlaveGeometry, Selection* inSelection) = 0;
	virtual void DrawRangeSlave(bool inHorizontal, const ZDC& inDC,
										ZCoord inOriginMaster, ZCoord inCellSizeMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Selection* inSelection) = 0;
	virtual int32 GetCellCount(bool inHorizontal) = 0;

	virtual int32 PixelToCell(bool inHorizontal, int32 inOriginCell, ZCoord inPixel, bool inRoundUp, bool inClamp = true) = 0;
	virtual ZCoord CellToPixel(bool inHorizontal, int32 inOriginCell, int32 inCell) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry_Fixed

class ZUIGridPane::Geometry_Fixed : public ZUIGridPane::Geometry
	{
public:
	Geometry_Fixed();
	virtual ~Geometry_Fixed();

// From ZUIGridPane::Geometry
	virtual int32 GetIterationCost(bool inHorizontal);
	virtual void DrawRangeMaster(bool inHorizontal, const ZDC& inDC,
										ZCoord inBeginCoordMaster, ZCoord inEndCoordMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inBeginCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Geometry* inSlaveGeometry, Selection* inSelection);
	virtual void DrawRangeSlave(bool inHorizontal, const ZDC& inDC,
										ZCoord inOriginMaster, ZCoord inCellSizeMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Selection* inSelection);

	virtual int32 PixelToCell(bool inHorizontal, int32 inOriginCell, ZCoord inPixel, bool inRoundUp, bool inClamp = true);
	virtual ZCoord CellToPixel(bool inHorizontal, int32 inOriginCell, int32 inCell);

// Stuff that must be overridden
	virtual ZCoord GetCellSize(bool inHorizontal) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry_Variable

class ZUIGridPane::Geometry_Variable : public ZUIGridPane::Geometry
	{
public:
	Geometry_Variable();
	virtual ~Geometry_Variable();

// From ZUIGridPane::Geometry
	virtual int32 GetIterationCost(bool inHorizontal);
	virtual void DrawRangeMaster(bool inHorizontal, const ZDC& inDC,
										ZCoord inBeginCoordMaster, ZCoord inEndCoordMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inBeginCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Geometry* inSlaveGeometry, Selection* inSelection);
	virtual void DrawRangeSlave(bool inHorizontal, const ZDC& inDC,
										ZCoord inOriginMaster, ZCoord inCellSizeMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Selection* inSelection);

	virtual int32 PixelToCell(bool inHorizontal, int32 inOriginCell, ZCoord inPixel, bool inRoundUp, bool inClamp = true);
	virtual ZCoord CellToPixel(bool inHorizontal, int32 inOriginCell, int32 inCell);

// Stuff that must be overridden
	virtual ZCoord GetCellSize(bool inHorizontal, int32 inCellNumber) = 0;
	};

// =================================================================================================

#endif // __ZUIGridPane__

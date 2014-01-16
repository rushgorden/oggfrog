#ifndef __NPainter__
#define __NPainter__ 1
#include "zconfig.h"

#include "ZDC.h"
#include "ZInterest.h"
#include "ZPane.h"
#include "ZRefCount.h"

class NPaintDataRep;
class ZTuple;

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine

class NPaintEngine : protected ZInterestClient
	{
public:
	class Host;
	class PaintState;
	class PaintState_Indirect;
	class Action;

	enum ToolType { eToolTypeArrow, eToolTypeText, eToolTypeFill, eToolTypeBrush, eToolTypeDropper,
				eToolTypeRect, eToolTypeOval, eToolTypePolygon,
				eToolTypeRectFrame, eToolTypeOvalFrame, eToolTypePolygonFrame,
				eToolTypeFreeform,
				eToolTypeLine, eToolTypeEraser, eToolTypeSelect, eToolTypeLasso };

	enum CursorName { eCursorNameUnknown, eCursorNameArrow, eCursorNameDragSelection, eCursorNameText, eCursorNameFill,
				eCursorNameDropper, eCursorNameCrossHairs, eCursorNamePaintBrush, eCursorNameEraser, eCursorNameSelect, eCursorNameLasso };

	NPaintEngine(Host* inHost, const ZRef<PaintState>& inPaintState, ZPoint inSize);
	NPaintEngine(Host* inHost, const ZRef<PaintState>& inPaintState, const ZRef<NPaintDataRep>& inPaintData);

	virtual ~NPaintEngine();

// From ZInterestClient
	virtual void InterestChanged(ZInterest* inInterest);

// Our protocol
	ToolType GetTool();

	long GetChangeCount();

	Host* GetHost();

	ZPoint GetSize();
	void Draw(const ZDC& inDC, ZPoint inLocation);
	bool Click(ZPoint inHostHitPoint, const ZEvent_Mouse& inEvent);
	bool KeyDown(const ZEvent_Key& inEvent);
	void Idle(const ZDC& inDC, ZPoint inLocation);
	void Activate(bool inIsNowActive);
	void SetupMenus(ZMenuSetup* inMenuSetup);
	bool MenuMessage(const ZMessage& inMessage);
	void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);
	bool GetCursor(ZPoint inHostLocation, ZCursor& outCursor);

	void EraseAll();

// Coordinate transforms
	ZPoint ToHost(ZPoint inEnginePoint);
	ZPoint FromHost(ZPoint inHostPoint);

	ZRect ToHost(const ZRect& inEngineRect);
	ZRect FromHost(const ZRect& inHostRect);

// Invalidation
	void InvalidateRgn(const ZDCRgn& inEngineRgn);

// Getting data
	ZRef<NPaintDataRep> GetPaintData();
	ZRef<NPaintDataRep> GetPaintData(const ZRect& iBounds);

// Selection management
	void GetSelectionForClipboard(ZTuple& outTuple, ZRef<NPaintDataRep>& outPaintData);
	ZRef<NPaintDataRep> GetSelectionPaintData();
	ZDCRgn ReplaceSelectionPaintData(ZRef<NPaintDataRep> inPaintData);
	ZDCRgn PastePaintData(ZRef<NPaintDataRep> inPaintData);
	bool HasSelection();

	ZDCRgn OffsetSelection(ZPoint inOffset);

	ZDCRgn SetPixelSelectionRgn(const ZDCRgn& inRgn);
	ZDCRgn SetPixelSelectionPoints(const vector<ZPoint>& inPixelSelectionPoints);
	ZDCRgn GetPixelSelectionRgn();

	ZDCRgn SetPixelSelectionFeedback(const vector<ZPoint>& inPixelSelectionFeedback);

	ZDCRgn PunchPixelSelectionIntoPixelDC();

	bool GetCursor(CursorName inCursorName, ZCursor& outCursor);

// Action management
	void PostAction(Action* inAction);

	void CommitLastAction();

// Access to the pixel DC
	const ZDC& GetPixelDC();

// Swapping DCs (undo support)
	ZDCRgn SwapDCs(ZDC& inOutDC);

	ZRef<NPaintEngine::PaintState> GetPaintState();

	void Resize(ZPoint iOffset, ZPoint iSize);

// Calls re TextPlane
	bool GetTextPlaneActive();
	bool GetTextPlaneCanUseTextTool();

//	bool GetPixelPlaneSelectionActive();

// A couple of utility routines
	static void sExtractMonoPatterns(const ZDCPixmap& inPixmap, vector<ZDCPattern>& outPatterns);
	static void sExtractColors(const ZDCPixmap& inPixmap, ZCoord inCountH, ZCoord inCountV, vector<ZRGBColor>& outColors);

protected:
	class Text;
	class TextPlane;

	ZDC fPixelDC;
	ZPoint fSize;

	vector<ZPoint> fPixelSelectionFeedback;

	ZDCPattern fPixelSelectionPattern;
	ZDCRgn fPixelSelectionRgn;
	ZPoint fPixelSelectionOffset;
	ZDCPixmap fPixelSelectionPixmap;

	TextPlane* fTextPlane;

	Host* fHost;
	ZRef<NPaintEngine::PaintState> fPaintState;

	ToolType fLastKnownTool;
	bool fLastKnownToolValid;

	bigtime_t fTimeOfLastTick;

	Action* fLastAction;
	bool fLastActionDone;

	long fChangeCount;

	class ActionInval;
	class ActionPaint;
	class ActionMouse;

	class MouseTracker_ActionMouse;
	class MouseTracker_ActionMouseMultiClick;

	class Action_SetSelection;
	class MouseTracker_SelectRect;
	class MouseTracker_SelectFreeform;

	class Action_SelectionMove;
	class MouseTracker_SelectionMove;
	class Action_TextResize;
	class MouseTracker_TextResize;

	class Action_Clear;
	class Action_Cut;
	class Action_Paste;

	class Action_EraseAll;

	class Action_PaintBucket;
	
	class Action_Rectangle;
	class Action_Oval;
	class Action_Line;
	class Action_Brush;
	class Action_Eraser;
	class Action_Polygon;

	class MouseTracker_Dropper;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Host

class NPaintEngine::Host
	{
public:
	virtual ZSubPane* GetPaintEnginePane() = 0;
	virtual bool GetPaintEngineActive() = 0;
	virtual ZPoint GetPaintEngineLocation() = 0;
	virtual void InvalidatePaintEngineRgn(const ZDCRgn& inInvalRgn) = 0;
	virtual void InvalidatePaintEngineCursor() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::PaintState

class NPaintEngine::PaintState : public ZRefCountedWithFinalization
	{
protected:
	PaintState();
	virtual ~PaintState();

public:
	virtual NPaintEngine::ToolType GetTool() = 0;

	virtual ZDCInk GetInk_Stroke() = 0;
	virtual ZDCRgn GetRgn_Pen() = 0;
	virtual ZDCRgn GetRgn_Eraser() = 0;
	virtual ZDCInk GetInk_Fill() = 0;
	virtual ZCoord GetWidth_Frame() = 0;
	virtual ZRGBColor GetColor_Text() = 0;

	virtual bool GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor);

	virtual void SetColor_Stroke(const ZRGBColor& inColor) = 0;

	virtual void SetTool(NPaintEngine::ToolType inTool) = 0;

	virtual ZInterest* GetInterest_All() = 0;
	virtual ZInterest* GetInterest_Tool() = 0;
	virtual ZInterest* GetInterest_TextColor() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::PaintState_Indirect

class NPaintEngine::PaintState_Indirect : public NPaintEngine::PaintState,
			protected ZInterestClient
	{
public:
	PaintState_Indirect(ZRef<NPaintEngine::PaintState> inRealPaintState);
	virtual ~PaintState_Indirect();

// From NPaintEngine::PaintState
	virtual NPaintEngine::ToolType GetTool();

	virtual ZDCInk GetInk_Stroke();
	virtual ZDCRgn GetRgn_Pen();
	virtual ZDCRgn GetRgn_Eraser();
	virtual ZDCInk GetInk_Fill();
	virtual ZCoord GetWidth_Frame();

	virtual bool GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor);

	virtual void SetColor_Stroke(const ZRGBColor& inColor);

	virtual void SetTool(NPaintEngine::ToolType inToolType);

	virtual ZInterest* GetInterest_All();
	virtual ZInterest* GetInterest_Tool();
	virtual ZInterest* GetInterest_TextColor();

// From ZInterestClient
	virtual void InterestChanged(ZInterest* inInterest);

// Our protocol
	void SetRealPaintState(ZRef<NPaintEngine::PaintState> inRealPaintState);
protected:
	ZRef<NPaintEngine::PaintState> fRealPaintState;
	ZInterest fInterest_All;
	ZInterest fInterest_Tool;
	ZInterest fInterest_TextColor;
	};

// =================================================================================================

#endif // __NPainter__

#include "NPainter.h"
#include "NPaintDataRep.h"

#include "ZDragClip.h"
#include "ZFakeWindow.h" // For ZMouseTracker
#include "ZFontMenus.h"
#include "ZMemoryBlock.h"
#include "ZTicks.h"
#include "ZTuple.h"
#include "ZUICartesianPlane.h"
#include "ZUITextEngine_Z.h"
#include "ZUtil_Graphics.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <Fonts.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Utility Routines

static bool sMungeProc_Rgn(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	if (static_cast<ZDCRgn*>(inRefCon)->Contains(hCoord, vCoord))
		inOutColor = ZRGBColor::sBlack;
	else
		inOutColor = ZRGBColor::sWhite;
	return true;
	}

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColor& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

static ZCursor sCursorFromRgn(const ZDCRgn& inRgn)
	{
	// Take a local copy
	ZDCRgn localRgn(inRgn);

	// Normalize it to place the top left corner at (0, 0)
	localRgn -= localRgn.Bounds().TopLeft();

	// Create the mono pixmap, the same size as the region
	ZDCPixmap monoPixmap(localRgn.Bounds().Size(),
		ZDCPixmapNS::eFormatStandard_Gray_1, ZRGBColor::sWhite);

	// Set every pixel in the region to black
	monoPixmap.Munge(sMungeProc_Rgn, &localRgn);

	// Instantiate the mask pixmap, all black so that the mono pixels invert
	ZDCPixmap maskPixmap(localRgn.Bounds().Size(),
		ZDCPixmapNS::eFormatStandard_Gray_1, ZRGBColor::sBlack);

	return ZCursor(ZDCPixmap(), monoPixmap, maskPixmap, -inRgn.Bounds().TopLeft());
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action

class NPaintEngine::Action
	{
public:
	Action();
	virtual ~Action();

	virtual void Commit(NPaintEngine* inEngine) = 0;
	virtual void Do(NPaintEngine* inEngine) = 0;
	virtual void Undo(NPaintEngine* inEngine) = 0;
	virtual void Redo(NPaintEngine* inEngine) = 0;

	virtual string GetUndoText();
	virtual string GetRedoText();
	};

NPaintEngine::Action::Action()
	{}

NPaintEngine::Action::~Action()
	{}

string NPaintEngine::Action::GetUndoText()
	{ return "Undo"; }

string NPaintEngine::Action::GetRedoText()
	{ return "Redo"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::ActionInval

class NPaintEngine::ActionInval : public NPaintEngine::Action
	{
public:
	ActionInval();
	virtual ~ActionInval();

// From NPaintEngine::Action
	virtual void Commit(NPaintEngine* inEngine);
	virtual void Do(NPaintEngine* inEngine);
	virtual void Undo(NPaintEngine* inEngine);
	virtual void Redo(NPaintEngine* inEngine);

// Our protocol
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);
	};

NPaintEngine::ActionInval::ActionInval()
	{}

NPaintEngine::ActionInval::~ActionInval()
	{}

void NPaintEngine::ActionInval::Commit(NPaintEngine* inEngine)
	{ inEngine->InvalidateRgn(this->CommitReturnRgn(inEngine)); }

void NPaintEngine::ActionInval::Do(NPaintEngine* inEngine)
	{ inEngine->InvalidateRgn(this->DoReturnRgn(inEngine)); }

void NPaintEngine::ActionInval::Undo(NPaintEngine* inEngine)
	{ inEngine->InvalidateRgn(this->UndoReturnRgn(inEngine)); }

void NPaintEngine::ActionInval::Redo(NPaintEngine* inEngine)
	{ inEngine->InvalidateRgn(this->RedoReturnRgn(inEngine)); }

ZDCRgn NPaintEngine::ActionInval::CommitReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionInval::DoReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionInval::UndoReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionInval::RedoReturnRgn(NPaintEngine* inEngine)
	{ return this->DoReturnRgn(inEngine); }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Text Declaration

static const ZCoord kHandleSize = 4;

class NPaintEngine::Text : protected ZUITextEngine::Host
	{
public:
	Text(TextPlane* inTextPlane);
	Text(TextPlane* inTextPlane, const string& inString,
		const ZDCFont& inFont, const ZRect& inBounds, const ZRGBColor& inColor);
	virtual ~Text();

// From ZUITextEngine::Host
	virtual ZSubPane* GetHostPane(ZUITextEngine* inTextEngine);
	virtual ZPoint GetEngineLocation(ZUITextEngine* inTextEngine);
	virtual void GetEngineSizes(ZUITextEngine* inTextEngine,
		ZPoint& outEngineSize, ZCoord& outEngineWidth);

	virtual ZDCRgn GetEngineClip(ZUITextEngine* inTextEngine);
	virtual bool GetEngineEditable(ZUITextEngine* inTextEngine);
	virtual bool GetEngineActive(ZUITextEngine* inTextEngine);

// Our protocol
	void GetData(string& outString, ZDCFont& outFont, ZRect& outBounds, ZRGBColor& outColor);
	ZDCFont GetFont();
	void SetFont(const ZDCFont& inFont);
	ZRGBColor GetTextColor();
	void SetTextColor(const ZRGBColor& inColor);

	long GetChangeCount();

	bool Contains(ZPoint inPaintEngineHitPoint);
	void Activate(bool inNowActive);
	void Draw(const ZDC& inDC);
	void Idle(const ZDC& inDC);
	bool Click(ZPoint inPaintEngineHitPoint, const ZEvent_Mouse& inEvent);
	bool KeyDown(const ZEvent_Key& inEvent);

	ZRect GetBounds()
		{ return fBounds; }
	ZDCRgn GetRgn();

	ZDCRgn Offset(ZPoint inDelta);

	ZRect CalcHandleBounds(size_t inHandleIndex);
	size_t FindHandle(ZPoint inPaintEngineHitPoint);
	ZDCRgn SetBounds(const ZRect& inOriginalBounds, ZPoint inDelta, size_t inHandleIndex);

	ZUITextEngine* GetTextEngine() { return fTextEngine; }

protected:
	TextPlane* fTextPlane;
	ZRect fBounds;
	ZUITextEngine_Z* fTextEngine;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::TextPlane Declaration

class NPaintEngine::TextPlane : protected ZUICartesianPlane
	{
public:
	TextPlane(NPaintEngine* inPaintEngine);
	TextPlane(NPaintEngine* inPaintEngine,
		const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
		const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors);

	virtual ~TextPlane();

	void Draw(const ZDC& inDC);
	void Click(ZPoint inPoint, const ZEvent_Mouse& inEvent);
	bool KeyDown(const ZEvent_Key& inEvent);
	void Idle(const ZDC& inDC, ZPoint inLocation);
	void Activate(bool inIsNowActive);
	void SetupMenus(ZMenuSetup* inMenuSetup);
	bool MenuMessage(const ZMessage& inMessage);
	bool GetCursor(ZPoint inHostLocation, ZCursor& outCursor);

	void Offset(ZPoint inOffset);
	ZDCRgn OffsetSelection(ZPoint inOffset);

	long GetChangeCount();

	bool HasSelection();
	ZDCRgn PasteDataAndSelectIt(const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
		const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors);

	ZDCRgn ReplaceSelection(const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
		const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors);

	ZDCRgn DeleteSelection();

	void GetData(vector<string>& outStrings, vector<ZDCFont>& outFonts,
		vector<ZRect>& outBounds, vector<ZRGBColor>& outColors);

	void GetSelectedData(vector<string>& outStrings, vector<ZDCFont>& outFonts,
		vector<ZRect>& outBounds, vector<ZRGBColor>& outColors);

	NPaintEngine* GetPaintEngine() { return fPaintEngine; }

	bool GetShowBorders();
	bool GetTextSelected(Text* inText);
	bool GetTextEditable(Text* inText);
	bool GetTextShowHandles(Text* inText);

	void InvalidateAllTextBoxes();
	void UpdateSelectedTextColor(ZRGBColor inColor);

// From ZUICartesianPlane
	virtual long IndexFromPoint(ZPoint inPoint);
	virtual void IndicesFromRegion(ZDCRgn inRgn, set<long>& outIndices);
	virtual ZPoint PointFromIndex(long inIndex);
	virtual ZDCRgn RegionFromIndex(long inIndex);

	virtual bool GetIndexSelected(long inIndex);
	virtual void SetIndexSelected(long inIndex, bool selectIt, bool inNotify);
	virtual void Invalidate(const ZDCRgn& inRgn);

	virtual long BringIndexToFront(long inIndex);

	virtual void GetSelectedIndices(set<long>& outSelection);
	virtual void SetSelectedIndices(const set<long>& inSelection, bool inNotify);
	virtual bool HandleDoubleClick(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool HandleBackgroundClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool HandleDrag(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	virtual ZPoint FromPane(ZPoint inPanePoint);

protected:
	vector<Text*> fVector_Text;
	set<Text*> fSet_SelectedText;

	NPaintEngine* fPaintEngine;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_TextResize

class NPaintEngine::Action_TextResize : public NPaintEngine::ActionInval
	{
public:
	Action_TextResize(NPaintEngine::Text* inText,
		const ZRect& inOriginalBounds, ZPoint inDelta, size_t inHandleIndex);

	virtual ~Action_TextResize();

// From NPaintEngine::ActionInval
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);

	virtual string GetUndoText();
	virtual string GetRedoText();

protected:
	NPaintEngine::Text* fText;
	ZPoint fDelta;
	ZRect fOriginalBounds;
	size_t fHandleIndex;
	};

NPaintEngine::Action_TextResize::Action_TextResize(NPaintEngine::Text* inText,
	const ZRect& inOriginalBounds, ZPoint inDelta, size_t inHandleIndex)
:	fText(inText),
	fOriginalBounds(inOriginalBounds),
	fDelta(inDelta),
	fHandleIndex(inHandleIndex)
	{}

NPaintEngine::Action_TextResize::~Action_TextResize()
	{}

ZDCRgn NPaintEngine::Action_TextResize::CommitReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_TextResize::DoReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_TextResize::RedoReturnRgn(NPaintEngine* inEngine)
	{ return fText->SetBounds(fOriginalBounds, fDelta, fHandleIndex); }

ZDCRgn NPaintEngine::Action_TextResize::UndoReturnRgn(NPaintEngine* inEngine)
	{ return fText->SetBounds(fOriginalBounds, ZPoint::sZero, fHandleIndex); }

string NPaintEngine::Action_TextResize::GetUndoText()
	{ return "Undo Resize Text"; }

string NPaintEngine::Action_TextResize::GetRedoText()
	{ return "Redo Resize Text"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_TextResize

class NPaintEngine::MouseTracker_TextResize : public ZMouseTracker
	{
public:
	MouseTracker_TextResize(NPaintEngine* inPaintEngine,
		ZPoint inStartPoint, NPaintEngine::Text* inText, size_t inHandleIndex);

	virtual ~MouseTracker_TextResize();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

protected:
	NPaintEngine* fPaintEngine;
	NPaintEngine::Text* fText;
	ZRect fOriginalBounds;
	size_t fHandleIndex;
	};

NPaintEngine::MouseTracker_TextResize::MouseTracker_TextResize(NPaintEngine* inPaintEngine,
	ZPoint inStartPoint, NPaintEngine::Text* inText, size_t inHandleIndex)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint)
	{
	fPaintEngine = inPaintEngine;
	fText = inText;
	fHandleIndex = inHandleIndex;
	fOriginalBounds = fText->GetBounds();
	}

NPaintEngine::MouseTracker_TextResize::~MouseTracker_TextResize()
	{
	}

void NPaintEngine::MouseTracker_TextResize::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inMoved || inPhase != ePhaseContinue)
		{
		ZDCRgn changedRgn = fText->SetBounds(fOriginalBounds, inNext - inAnchor, fHandleIndex);
		fPaintEngine->InvalidateRgn(changedRgn);
		if (inPhase == ePhaseEnd)
			{
			fPaintEngine->PostAction(new Action_TextResize(fText, fOriginalBounds,
				inNext - inAnchor, fHandleIndex));
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_SelectionMove

class NPaintEngine::Action_SelectionMove : public NPaintEngine::ActionInval
	{
public:
	Action_SelectionMove(ZPoint inDelta, bool inApplyDeltaOnDo);
	virtual ~Action_SelectionMove();

// From NPaintEngine::ActionInval
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);

	virtual string GetUndoText();
	virtual string GetRedoText();

	ZPoint GetDelta() { return fDelta; }
	void SetDelta(ZPoint inDelta) { fDelta = inDelta; }
protected:
	ZPoint fDelta;
	bool fApplyDeltaOnDo;
	};

NPaintEngine::Action_SelectionMove::Action_SelectionMove(ZPoint inDelta, bool inApplyDeltaOnDo)
:	fDelta(inDelta), fApplyDeltaOnDo(inApplyDeltaOnDo)
	{}

NPaintEngine::Action_SelectionMove::~Action_SelectionMove()
	{
	}

ZDCRgn NPaintEngine::Action_SelectionMove::CommitReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_SelectionMove::DoReturnRgn(NPaintEngine* inEngine)
	{
	if (fApplyDeltaOnDo)
		return inEngine->OffsetSelection(fDelta);
	// If fApplyDeltaOnDo is false then by the time we are posted, the selection
	// has already been moved by our delta as a result of our tracking the mouse.
	return ZDCRgn();
	}

ZDCRgn NPaintEngine::Action_SelectionMove::RedoReturnRgn(NPaintEngine* inEngine)
	{ return inEngine->OffsetSelection(fDelta); }

ZDCRgn NPaintEngine::Action_SelectionMove::UndoReturnRgn(NPaintEngine* inEngine)
	{ return inEngine->OffsetSelection(-fDelta); }

string NPaintEngine::Action_SelectionMove::GetUndoText()
	{ return "Undo Move Selection"; }

string NPaintEngine::Action_SelectionMove::GetRedoText()
	{ return "Redo Move Selection"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_SelectionMove

class NPaintEngine::MouseTracker_SelectionMove : public ZMouseTracker
	{
public:
	MouseTracker_SelectionMove(NPaintEngine* inPaintEngine,
		ZPoint inStartPoint, bool inLeaveSelectionBehind);

	virtual ~MouseTracker_SelectionMove();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);
protected:
	NPaintEngine* fPaintEngine;
	};

NPaintEngine::MouseTracker_SelectionMove::MouseTracker_SelectionMove(NPaintEngine* inPaintEngine,
	ZPoint inStartPoint, bool inLeaveSelectionBehind)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine)
	{
	fPaintEngine->CommitLastAction();
	if (inLeaveSelectionBehind)
		fPaintEngine->PunchPixelSelectionIntoPixelDC();
	}

NPaintEngine::MouseTracker_SelectionMove::~MouseTracker_SelectionMove()
	{
	}

void NPaintEngine::MouseTracker_SelectionMove::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inMoved || inPhase != ePhaseContinue)
		{
		ZDCRgn changedRgn = fPaintEngine->OffsetSelection(inNext - inPrevious);
		fPaintEngine->InvalidateRgn(changedRgn);
		if (inPhase == ePhaseEnd)
			fPaintEngine->PostAction(new Action_SelectionMove(inNext - inAnchor, false));
		}
	}

bool NPaintEngine::MouseTracker_SelectionMove::GetCursor(ZPoint inPanePoint, ZCursor& outCursor)
	{ return fPaintEngine->GetCursor(NPaintEngine::eCursorNameArrow, outCursor); }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Text Implementation

NPaintEngine::Text::Text(TextPlane* inTextPlane)
	{
	fTextPlane = inTextPlane;
	fBounds.left = 30;
	fBounds.top = 70;
	fBounds.right = 180;
	fBounds.bottom = 90;

//	fTextEngine = new ZUITextEngine_TextEdit(ZDCFont());
	fTextEngine = new ZUITextEngine_Z(ZDCFont());
	fTextEngine->Attached(this);
	fTextEngine->SetText("this is a test");
	}

NPaintEngine::Text::Text(TextPlane* inTextPlane,
	const string& inString, const ZDCFont& inFont, const ZRect& inBounds, const ZRGBColor& inColor)
	{
	fTextPlane = inTextPlane;
	fBounds = inBounds;
	fTextEngine = new ZUITextEngine_Z(ZDCFont());
	fTextEngine->Attached(this);
	fTextEngine->SetText(inString);
	fTextEngine->SetFont(inFont);
	fTextEngine->SetTextColor(inColor);
	}

NPaintEngine::Text::~Text()
	{
	fTextEngine->Detached(this);
	delete fTextEngine;
	}

ZSubPane* NPaintEngine::Text::GetHostPane(ZUITextEngine* inTextEngine)
	{ return fTextPlane->GetPaintEngine()->GetHost()->GetPaintEnginePane(); }

ZPoint NPaintEngine::Text::GetEngineLocation(ZUITextEngine* inTextEngine)
	{ return fTextPlane->GetPaintEngine()->ToHost(fBounds.TopLeft()); }

void NPaintEngine::Text::GetEngineSizes(ZUITextEngine* inTextEngine,
	ZPoint& outEngineSize, ZCoord& outEngineWidth)
	{
	outEngineSize = fBounds.Size();
	outEngineWidth = fBounds.Width();
	}

ZDCRgn NPaintEngine::Text::GetEngineClip(ZUITextEngine* inTextEngine)
	{
	return fBounds;
 //	return fPaintEngine->GetHost()->GetPaintEngineClip() & fPaintEngine->ToHost(fBounds);
	}

bool NPaintEngine::Text::GetEngineEditable(ZUITextEngine* inTextEngine)
	{
	return fTextPlane->GetTextEditable(this);
	}

bool NPaintEngine::Text::GetEngineActive(ZUITextEngine* inTextEngine)
	{
	return fTextPlane->GetTextEditable(this);
	}

void NPaintEngine::Text::GetData(string& outString, ZDCFont& outFont,
	ZRect& outBounds, ZRGBColor& outColor)
	{
	outString = fTextEngine->GetText();
	outFont = fTextEngine->GetFont();
	outBounds = fBounds;
	outColor = fTextEngine->GetTextColor();
	}

ZDCFont NPaintEngine::Text::GetFont()
	{ return fTextEngine->GetFont(); }

void NPaintEngine::Text::SetFont(const ZDCFont& inFont)
	{ fTextEngine->SetFont(inFont); }

long NPaintEngine::Text::GetChangeCount()
	{ return fTextEngine->GetUserChangeCount(); }

ZRGBColor NPaintEngine::Text::GetTextColor()
	{ return fTextEngine->GetTextColor(); }

void NPaintEngine::Text::SetTextColor(const ZRGBColor& inColor)
	{ fTextEngine->SetTextColor(inColor); }

bool NPaintEngine::Text::Contains(ZPoint inPaintEngineHitPoint)
	{
	return fBounds.Inset(-kHandleSize, -kHandleSize).Contains(inPaintEngineHitPoint);
	}

void NPaintEngine::Text::Activate(bool inNowActive)
	{
	fTextEngine->Activate(inNowActive);
	}

void NPaintEngine::Text::Draw(const ZDC& inDC)
	{
	ZDC localDC = inDC;
	localDC.PenNormal();
	if (fTextPlane->GetShowBorders())
		{
		if (fTextPlane->GetTextSelected(this))
			localDC.SetInk(ZRGBColor::sBlack);
		else
			localDC.SetInk(ZRGBColor::sWhite / 2);

		localDC.SetPenWidth(kHandleSize);
		localDC.Frame(fBounds.Inset(-kHandleSize,-kHandleSize));

		if (fTextPlane->GetTextShowHandles(this))
			{
			localDC.SetInk(ZRGBColor::sWhite / 2);
			for (size_t x = 0; x < 9; ++x)
				localDC.Fill(this->CalcHandleBounds(x));
			}
		}
	localDC.OffsetOrigin(fBounds.TopLeft());
	localDC.SetClip(localDC.GetClip() & (fBounds - fBounds.TopLeft()));
	fTextEngine->Draw(localDC);
	}

void NPaintEngine::Text::Idle(const ZDC& inDC)
	{
	ZDC localDC = inDC;
	localDC.OffsetOrigin(fBounds.TopLeft());
	localDC.SetClip(localDC.GetClip() & (fBounds - fBounds.TopLeft()));
	fTextEngine->Idle(localDC, true);
	}

bool NPaintEngine::Text::Click(ZPoint inPaintEngineHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fTextPlane->GetTextEditable(this))
		{
		if (fBounds.Contains(inPaintEngineHitPoint))
			{
			return fTextEngine->Click(
				fTextPlane->GetPaintEngine()->ToHost(inPaintEngineHitPoint), inEvent);
			}
		}

	if (fTextPlane->GetTextShowHandles(this))
		{
		size_t hitHandle = this->FindHandle(inPaintEngineHitPoint);
		if (hitHandle < 9)
			{
			ZMouseTracker* theTracker = new NPaintEngine::MouseTracker_TextResize(
				fTextPlane->GetPaintEngine(),
				fTextPlane->GetPaintEngine()->ToHost(inPaintEngineHitPoint),
				this,
				hitHandle);
			theTracker->Install();
			return true;
			}
		}

	return false;
	}

bool NPaintEngine::Text::KeyDown(const ZEvent_Key& inEvent)
	{
	return fTextEngine->KeyDown(inEvent);
	}

ZDCRgn NPaintEngine::Text::GetRgn()
	{
	ZRect theBounds = fBounds;
	if (fTextPlane->GetShowBorders())
		theBounds = theBounds.Inset(-kHandleSize, -kHandleSize);
	return theBounds;
	}

ZDCRgn NPaintEngine::Text::Offset(ZPoint inDelta)
	{
	ZDCRgn changedRgn;
	if (inDelta != ZPoint::sZero)
		{
		changedRgn = fBounds.Inset(-kHandleSize, -kHandleSize);
		fBounds += inDelta;
		changedRgn |= fBounds.Inset(-kHandleSize, -kHandleSize);
		}
	 return changedRgn;
	}

ZRect NPaintEngine::Text::CalcHandleBounds(size_t inHandleIndex)
	{ return ZUtil_Graphics::sCalcHandleBounds9(fBounds, kHandleSize, inHandleIndex); }

size_t NPaintEngine::Text::FindHandle(ZPoint inPaintEngineHitPoint)
	{
	for (size_t x = 0; x < 9; ++x)
		{
		if (this->CalcHandleBounds(x).Contains(inPaintEngineHitPoint))
			return x;
		}
	return 9;
	}

ZDCRgn NPaintEngine::Text::SetBounds(
	const ZRect& inOriginalBounds, ZPoint inDelta, size_t inHandleIndex)
	{
	ZRect newBounds = inOriginalBounds;
	if (inHandleIndex < 9 && inHandleIndex != 4)
		{
		// Find the horizontal portion
		switch (inHandleIndex % 3)
			{
			case 0:
				newBounds.left = min(newBounds.left + inDelta.h, newBounds.right - 4);
				break;
			case 1:
				break;
			case 2:
				newBounds.right = max(newBounds.right + inDelta.h, newBounds.left + 4);
				break;
			}

		// Find the vertical portion
		switch (inHandleIndex / 3)
			{
			case 0:
				newBounds.top = min(newBounds.top + inDelta.v, newBounds.bottom - 4);
				break;
			case 1:
				break;
			case 2:
				newBounds.bottom = max(newBounds.bottom + inDelta.v, newBounds.top + 4);
				break;
			}
		}

	ZDCRgn changedRgn;
	if (fBounds != newBounds)
		{
		changedRgn = fBounds.Inset(-kHandleSize, -kHandleSize);
		fBounds = newBounds;
		changedRgn |= fBounds.Inset(-kHandleSize, -kHandleSize);
		}
	return changedRgn;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::TextPlane Implementation

NPaintEngine::TextPlane::TextPlane(NPaintEngine* inPaintEngine)
:	fPaintEngine(inPaintEngine)
	{}

NPaintEngine::TextPlane::TextPlane(NPaintEngine* inPaintEngine,
	const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
	const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors)
:	fPaintEngine(inPaintEngine)
	{
	for (size_t x = 0; x < inStrings.size(); ++x)
		{
		Text* newText = new Text(this, inStrings[x], inFonts[x], inBounds[x], inColors[x]);
		fVector_Text.push_back(newText);
		}
	}

NPaintEngine::TextPlane::~TextPlane()
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		delete fVector_Text[x];
	}

void NPaintEngine::TextPlane::Draw(const ZDC& inDC)
	{
	ZDC localDC = inDC;
	for (size_t x = fVector_Text.size(); x; --x)
		fVector_Text[x-1]->Draw(localDC);
	}

void NPaintEngine::TextPlane::Click(ZPoint inPoint, const ZEvent_Mouse& inEvent)
	{
	long hitIndex = this->IndexFromPoint(inPoint);
	if (hitIndex >= 0 && hitIndex < fVector_Text.size())
		{
		if (fSet_SelectedText.size() == 1)
			{
			if (fSet_SelectedText.find(fVector_Text[hitIndex]) != fSet_SelectedText.end())
				{
				if (fVector_Text[hitIndex]->Click(inPoint, inEvent))
					return;
				}
			}
		}
	this->ZUICartesianPlane::Click(inPoint, inEvent);
	}

bool NPaintEngine::TextPlane::KeyDown(const ZEvent_Key& inEvent)
	{
	if (fSet_SelectedText.size() == 1)
		{
		Text* theActiveText = *fSet_SelectedText.begin();
		return theActiveText->KeyDown(inEvent);
		}
	return false;
	}

void NPaintEngine::TextPlane::Idle(const ZDC& inDC, ZPoint inLocation)
	{
	if (fPaintEngine->GetTextPlaneCanUseTextTool())
		{
		if (fSet_SelectedText.size() == 1)
			{
			Text* theActiveText = *fSet_SelectedText.begin();
			ZDC localDC = inDC;
			localDC.OffsetOrigin(inLocation);
			theActiveText->Idle(localDC);
			}
		}
	}

void NPaintEngine::TextPlane::Activate(bool inIsNowActive)
	{
	if (fPaintEngine->GetTextPlaneCanUseTextTool())
		{
		if (fSet_SelectedText.size() == 1)
			{
			Text* theActiveText = *fSet_SelectedText.begin();
			theActiveText->Activate(inIsNowActive);
			}
		}
	}

void NPaintEngine::TextPlane::SetupMenus(ZMenuSetup* inMenuSetup)
	{
	if (fPaintEngine->GetTextPlaneCanUseTextTool())
		{
		if (fSet_SelectedText.size() == 1)
			{
			Text* theActiveText = *fSet_SelectedText.begin();
			theActiveText->GetTextEngine()->SetupMenus(inMenuSetup);
			}
		}
	if (fSet_SelectedText.size() > 0)
		{
		vector<ZDCFont> theFonts;
		for (set<Text*>::iterator i = fSet_SelectedText.begin(); i != fSet_SelectedText.end(); ++i)
			theFonts.push_back((*i)->GetFont());
		ZFontMenus::sSetupMenus(inMenuSetup, theFonts);
		}
	}

bool NPaintEngine::TextPlane::MenuMessage(const ZMessage& inMessage)
	{
	if (fPaintEngine->GetTextPlaneCanUseTextTool())
		{
		if (fSet_SelectedText.size() == 1)
			{
			Text* theActiveText = *fSet_SelectedText.begin();
			if (theActiveText->GetTextEngine()->MenuMessage(inMessage))
				return true;
			}
		}

	if (fSet_SelectedText.size() > 0)
		{
		switch (inMessage.GetInt32("menuCommand"))
			{
			case mcFontItem:
				{
				string theFontName = inMessage.GetString("fontName");
				for (set<Text*>::iterator i = fSet_SelectedText.begin();
					i != fSet_SelectedText.end(); ++i)
					{
					ZDCFont theFont = (*i)->GetFont();
					theFont.SetName(theFontName);
					(*i)->SetFont(theFont);
					}
				return true;
				}
			case mcSizeItem:
				{
				int32 fontSize = inMessage.GetInt32("fontSize");
				for (set<Text*>::iterator i = fSet_SelectedText.begin();
					i != fSet_SelectedText.end(); ++i)
					{
					ZDCFont theFont = (*i)->GetFont();
					theFont.SetSize(fontSize);
					(*i)->SetFont(theFont);
					}
				return true;
				}
			case mcStyleItem:
				{
				int32 fontStyle = inMessage.GetInt32("fontStyle");
				for (set<Text*>::iterator i = fSet_SelectedText.begin();
					i != fSet_SelectedText.end(); ++i)
					{
					ZDCFont theFont = (*i)->GetFont();
					if (fontStyle == ZDCFont::normal)
						theFont.SetStyle(ZDCFont::normal);
					else
						theFont.SetStyle(theFont.GetStyle() ^ fontStyle);
					(*i)->SetFont(theFont);
					}
				return true;
				}
			}
		}
	return false;
	}

bool NPaintEngine::TextPlane::GetCursor(ZPoint inHostLocation, ZCursor& outCursor)
	{
	return false;
	}

void NPaintEngine::TextPlane::Offset(ZPoint inOffset)
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		fVector_Text[x]->Offset(inOffset);
	}

ZDCRgn NPaintEngine::TextPlane::OffsetSelection(ZPoint inOffset)
	{
	ZDCRgn changedRgn;
	for (set<Text*>::iterator i = fSet_SelectedText.begin(); i != fSet_SelectedText.end(); ++i)
		changedRgn |= (*i)->Offset(inOffset);
	return changedRgn;
	}

long NPaintEngine::TextPlane::GetChangeCount()
	{
	long changeCount = 0;
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		changeCount += fVector_Text[x]->GetChangeCount();
	return changeCount;
	}

bool NPaintEngine::TextPlane::HasSelection()
	{
	return fSet_SelectedText.size() != 0;
	}

ZDCRgn NPaintEngine::TextPlane::PasteDataAndSelectIt(
	const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
	const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors)
	{
	ZDCRgn changedRgn;
	for (set<Text*>::iterator i = fSet_SelectedText.begin(); i != fSet_SelectedText.end(); ++i)
		changedRgn |= (*i)->GetRgn();
	fSet_SelectedText.erase(fSet_SelectedText.begin(), fSet_SelectedText.end());

	for (size_t x = inStrings.size(); x > 0; --x)
		{
		Text* newText = new Text(this, inStrings[x-1], inFonts[x-1], inBounds[x-1], inColors[x-1]);
		fVector_Text.insert(fVector_Text.begin(), newText);
		fSet_SelectedText.insert(newText);
		changedRgn |= newText->GetRgn();
		}

	return changedRgn;
	}

ZDCRgn NPaintEngine::TextPlane::ReplaceSelection(
	const vector<string>& inStrings, const vector<ZDCFont>& inFonts,
	const vector<ZRect>& inBounds, const vector<ZRGBColor>& inColors)
	{
	ZDCRgn changedRgn = this->DeleteSelection();
	changedRgn |= this->PasteDataAndSelectIt(inStrings, inFonts, inBounds, inColors);
	return changedRgn;
	}

ZDCRgn NPaintEngine::TextPlane::DeleteSelection()
	{
	ZDCRgn changedRgn;
	for (size_t x = 0; x < fVector_Text.size();)
		{
		set<Text*>::iterator theIter = fSet_SelectedText.find(fVector_Text[x]);
		if (theIter != fSet_SelectedText.end())
			{
			changedRgn |= fVector_Text[x]->GetRgn();

			fSet_SelectedText.erase(theIter);
			delete fVector_Text[x];
			fVector_Text.erase(fVector_Text.begin() + x);
			}
		else
			++x;
		}
	return changedRgn;
	}

void NPaintEngine::TextPlane::GetData(vector<string>& outStrings, vector<ZDCFont>& outFonts,
	vector<ZRect>& outBounds, vector<ZRGBColor>& outColors)
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		{
		string theString;
		ZDCFont theFont;
		ZRect theBounds;
		ZRGBColor theColor;
		fVector_Text[x]->GetData(theString, theFont, theBounds, theColor);
		outStrings.push_back(theString);
		outFonts.push_back(theFont);
		outBounds.push_back(theBounds);
		outColors.push_back(theColor);
		}
	}

void NPaintEngine::TextPlane::GetSelectedData(
	vector<string>& outStrings, vector<ZDCFont>& outFonts,
	vector<ZRect>& outBounds, vector<ZRGBColor>& outColors)
	{
	for (set<Text*>::iterator i = fSet_SelectedText.begin(); i != fSet_SelectedText.end(); ++i)
		{
		string theString;
		ZDCFont theFont;
		ZRect theBounds;
		ZRGBColor theColor;
		(*i)->GetData(theString, theFont, theBounds, theColor);
		outStrings.push_back(theString);
		outFonts.push_back(theFont);
		outBounds.push_back(theBounds);
		outColors.push_back(theColor);
		}
	}

bool NPaintEngine::TextPlane::GetShowBorders()
	{
	return fPaintEngine->GetTextPlaneActive();
	}

bool NPaintEngine::TextPlane::GetTextSelected(Text* inText)
	{
	return fSet_SelectedText.find(inText) != fSet_SelectedText.end();
	}

bool NPaintEngine::TextPlane::GetTextEditable(Text* inText)
	{
	return fPaintEngine->GetTextPlaneCanUseTextTool()
		&& fSet_SelectedText.size() == 1
		&& (fSet_SelectedText.find(inText) != fSet_SelectedText.end());
	}

bool NPaintEngine::TextPlane::GetTextShowHandles(Text* inText)
	{
	return fSet_SelectedText.size() == 1
		&& (fSet_SelectedText.find(inText) != fSet_SelectedText.end());
	}

void NPaintEngine::TextPlane::InvalidateAllTextBoxes()
	{
	ZDCRgn theRgn;
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		theRgn |= fVector_Text[x]->GetRgn();
	fPaintEngine->InvalidateRgn(theRgn);
	}

void NPaintEngine::TextPlane::UpdateSelectedTextColor(ZRGBColor inColor)
	{
	if (fSet_SelectedText.size() > 0)
		{
		for (set<Text *>::iterator i = fSet_SelectedText.begin(); i != fSet_SelectedText.end(); ++i)
			(*i)->SetTextColor(inColor);
		}
	}

long NPaintEngine::TextPlane::IndexFromPoint(ZPoint inPoint)
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		{
		if (fVector_Text[x]->Contains(inPoint))
			return x;
		}
	return -1;
	}

void NPaintEngine::TextPlane::IndicesFromRegion(ZDCRgn inRgn, set<long>& outIndices)
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		{
		if (inRgn & fVector_Text[x]->GetRgn())
			outIndices.insert(x);
		}
	}

ZPoint NPaintEngine::TextPlane::PointFromIndex(long inIndex)
	{
	ZAssert(inIndex >= 0 && inIndex < fVector_Text.size());
	return fVector_Text[inIndex]->GetBounds().TopLeft();
	}

ZDCRgn NPaintEngine::TextPlane::RegionFromIndex(long inIndex)
	{
	ZAssert(inIndex >= 0 && inIndex < fVector_Text.size());
	return fVector_Text[inIndex]->GetRgn();
	}

bool NPaintEngine::TextPlane::GetIndexSelected(long inIndex)
	{
	ZAssert(inIndex >= 0 && inIndex < fVector_Text.size());
	return fSet_SelectedText.find(fVector_Text[inIndex]) != fSet_SelectedText.end();
	}

void NPaintEngine::TextPlane::SetIndexSelected(long inIndex, bool selectIt, bool inNotify)
	{
	Text* theText = fVector_Text[inIndex];
	set<Text*>::iterator foundIter = fSet_SelectedText.find(theText);
	bool currentlySelected =  foundIter != fSet_SelectedText.end();
	if (currentlySelected != selectIt)
		{
		Text* priorSingleSelection = nil;
		if (fSet_SelectedText.size() == 1)
			priorSingleSelection = *fSet_SelectedText.begin();
		if (priorSingleSelection)
			this->Invalidate(priorSingleSelection->GetRgn());

		if (selectIt)
			fSet_SelectedText.insert(theText);
		else
			fSet_SelectedText.erase(foundIter);

		this->Invalidate(theText->GetRgn());
		}
	}

void NPaintEngine::TextPlane::Invalidate(const ZDCRgn& inRgn)
	{
	fPaintEngine->InvalidateRgn(inRgn);
	}

long NPaintEngine::TextPlane::BringIndexToFront(long inIndex)
	{
	if (inIndex != 0)
		{
		Text* theText = fVector_Text[inIndex];
		fVector_Text.erase(fVector_Text.begin() + inIndex);
		fVector_Text.insert(fVector_Text.begin(), theText);
		}
	return 0;
	}

void NPaintEngine::TextPlane::GetSelectedIndices(set<long>& outSelection)
	{
	for (size_t x = 0; x < fVector_Text.size(); ++x)
		{
		if (fSet_SelectedText.find(fVector_Text[x]) != fSet_SelectedText.end())
			outSelection.insert(x);
		}
	}

void NPaintEngine::TextPlane::SetSelectedIndices(const set<long>& inSelection, bool inNotify)
	{
	Text* priorSingleSelection = nil;
	if (fSet_SelectedText.size() == 1)
		priorSingleSelection = *fSet_SelectedText.begin();

	for (size_t x = 0; x < fVector_Text.size(); ++x)
		{
		Text* currentText = fVector_Text[x];
		set<Text*>::iterator selectedTextIter = fSet_SelectedText.find(currentText);
		set<long>::const_iterator selectedIndexIter = inSelection.find(x);
		if ((selectedTextIter == fSet_SelectedText.end())
			!= (selectedIndexIter == inSelection.end()))
			{
			if (selectedTextIter == fSet_SelectedText.end())
				fSet_SelectedText.insert(currentText);
			else
				fSet_SelectedText.erase(selectedTextIter);
			this->Invalidate(currentText->GetRgn());
			}
		}

	Text* postSingleSelection = nil;
	if (fSet_SelectedText.size() == 1)
		postSingleSelection = *fSet_SelectedText.begin();

	if (priorSingleSelection != postSingleSelection)
		{
		if (priorSingleSelection)
			this->Invalidate(priorSingleSelection->GetRgn());
		if (postSingleSelection)
			this->Invalidate(postSingleSelection->GetRgn());
		}
	}

bool NPaintEngine::TextPlane::HandleDoubleClick(
	long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}

bool NPaintEngine::TextPlane::HandleBackgroundClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fPaintEngine->GetTextPlaneCanUseTextTool() && fSet_SelectedText.size() == 0)
		{
		Text* newText = new Text(this,
			string(),
			ZDCFont::sApp9,
			ZRect(inHitPoint.h - 8, inHitPoint.v - 8, inHitPoint.h, inHitPoint.v),
			fPaintEngine->GetPaintState()->GetColor_Text());

		fVector_Text.insert(fVector_Text.begin(), newText);
		fSet_SelectedText.insert(newText);
		this->Invalidate(newText->GetRgn());
		ZMouseTracker* theTracker = new NPaintEngine::MouseTracker_TextResize(
			fPaintEngine, fPaintEngine->ToHost(inHitPoint), newText, 8);
		theTracker->Install();
		}
	else
		{
		ZMouseTracker* theTracker = new TrackerBackground(
			fPaintEngine->GetHost()->GetPaintEnginePane(),
			this, fPaintEngine->ToHost(inHitPoint), nil);
		theTracker->Install();
		}
	return true;
	}

bool NPaintEngine::TextPlane::HandleDrag(
	long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZMouseTracker* theTracker = new MouseTracker_SelectionMove(
		fPaintEngine, fPaintEngine->ToHost(inHitPoint), false);
	theTracker->Install();
	return true;
	}

ZPoint NPaintEngine::TextPlane::FromPane(ZPoint inPanePoint)
	{
	return fPaintEngine->FromHost(inPanePoint);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::ActionPaint

class NPaintEngine::ActionPaint : public NPaintEngine::ActionInval
	{
public:
	ActionPaint();
	virtual ~ActionPaint();

// From NPaintEngine::ActionInval
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);

// Our protocol
	virtual ZDCRgn CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	};

NPaintEngine::ActionPaint::ActionPaint()
	{}

NPaintEngine::ActionPaint::~ActionPaint()
	{}

ZDCRgn NPaintEngine::ActionPaint::CommitReturnRgn(NPaintEngine* inEngine)
	{ return this->CommitWithDC(inEngine, inEngine->GetPixelDC()); }

ZDCRgn NPaintEngine::ActionPaint::DoReturnRgn(NPaintEngine* inEngine)
	{ return this->DoWithDC(inEngine, inEngine->GetPixelDC()); }

ZDCRgn NPaintEngine::ActionPaint::UndoReturnRgn(NPaintEngine* inEngine)
	{ return this->UndoWithDC(inEngine, inEngine->GetPixelDC()); }

ZDCRgn NPaintEngine::ActionPaint::RedoReturnRgn(NPaintEngine* inEngine)
	{ return this->RedoWithDC(inEngine, inEngine->GetPixelDC()); }

ZDCRgn NPaintEngine::ActionPaint::CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionPaint::DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionPaint::UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionPaint::RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return this->DoWithDC(inEngine, inPixelDC); }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::ActionMouse

class NPaintEngine::ActionMouse : public NPaintEngine::ActionPaint
	{
public:
	ActionMouse(const ZDC& inOriginalDC);
	virtual ~ActionMouse();

// From NPaintEngine::ActionPaint
	virtual ZDCRgn CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);

// Our protocol, called by simple mouse tracker
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);

// Our protocol, called by multi click mouse tracker
	virtual ZDCRgn MouseDown(ZDC& inTargetDC, ZPoint inLocation);
	virtual ZDCRgn MouseUp(ZDC& inTargetDC, ZPoint inLocation);
	virtual ZDCRgn Moved(ZDC& inTargetDC, ZPoint inLocation);
	virtual ZDCRgn Finished(ZDC& inTargetDC);

protected:
	ZDC fOriginalDC;
	};

NPaintEngine::ActionMouse::ActionMouse(const ZDC& inOriginalDC)
	{
	// Create the DC which holds the snapshot of the pixels before we start painting
	fOriginalDC = ZDC_Off(inOriginalDC, true);

	// Copy the snapshot pixels across
	ZRect sourceRect(inOriginalDC.GetClip().Bounds());
	fOriginalDC.CopyFrom(sourceRect.TopLeft(), inOriginalDC, sourceRect);
	}

NPaintEngine::ActionMouse::~ActionMouse()
	{}

ZDCRgn NPaintEngine::ActionMouse::CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionMouse::DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::ActionMouse::UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return inEngine->SwapDCs(fOriginalDC); }

ZDCRgn NPaintEngine::ActionMouse::RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return inEngine->SwapDCs(fOriginalDC); }

ZDCRgn NPaintEngine::ActionMouse::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	ZUnimplemented();
	return ZDCRgn();
	}

ZDCRgn NPaintEngine::ActionMouse::MouseUp(ZDC& inTargetDC, ZPoint inLocation)
	{
//	ZUnimplemented();
	return ZDCRgn();
	}

ZDCRgn NPaintEngine::ActionMouse::MouseDown(ZDC& inTargetDC, ZPoint inLocation)
	{
//	ZUnimplemented();
	return ZDCRgn();
	}

ZDCRgn NPaintEngine::ActionMouse::Moved(ZDC& inTargetDC, ZPoint inLocation)
	{
//	ZUnimplemented();
	return ZDCRgn();
	}

ZDCRgn NPaintEngine::ActionMouse::Finished(ZDC& inTargetDC)
	{
//	ZUnimplemented();
	return ZDCRgn();
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_ActionMouse

class NPaintEngine::MouseTracker_ActionMouse : public ZMouseTracker
	{
public:
	MouseTracker_ActionMouse(NPaintEngine* inPaintEngine,
		ZPoint inStartPoint, const ZDC& inOriginalDC,
		NPaintEngine::ActionMouse* inActionMouse, const ZCursor& inCursor);
	virtual ~MouseTracker_ActionMouse();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);
protected:
	NPaintEngine* fPaintEngine;
	ZDC fTargetDC;
	NPaintEngine::ActionMouse* fActionMouse;
	ZCursor fCursor;
	};

NPaintEngine::MouseTracker_ActionMouse::MouseTracker_ActionMouse(NPaintEngine* inPaintEngine,
	ZPoint inStartPoint, const ZDC& inOriginalDC,
	NPaintEngine::ActionMouse* inActionMouse, const ZCursor& inCursor)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine), fActionMouse(inActionMouse), fCursor(inCursor)
	{
	fPaintEngine->CommitLastAction();
	fTargetDC = inOriginalDC;
	}

NPaintEngine::MouseTracker_ActionMouse::~MouseTracker_ActionMouse()
	{
	ZAssert(!fActionMouse);
	}

void NPaintEngine::MouseTracker_ActionMouse::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inMoved || inPhase != ePhaseContinue)
		{
		ZDC targetDC(fTargetDC);

		ZDCRgn changedRgn = fActionMouse->DoIt(targetDC,
			fPaintEngine->FromHost(inAnchor),
			fPaintEngine->FromHost(inPrevious),
			fPaintEngine->FromHost(inNext));

		fPaintEngine->InvalidateRgn(changedRgn);
		if (inPhase == ePhaseEnd)
			{
			fPaintEngine->PostAction(fActionMouse);
			fActionMouse = nil;
			}
		}
	}

bool NPaintEngine::MouseTracker_ActionMouse::GetCursor(ZPoint inPanePoint, ZCursor& outCursor)
	{
	outCursor = fCursor;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_ActionMouseMultiClick

class NPaintEngine::MouseTracker_ActionMouseMultiClick : public ZMouseTracker
	{
public:
	MouseTracker_ActionMouseMultiClick(NPaintEngine* inPaintEngine,
		ZPoint inStartPoint, const ZDC& inOriginalDC,
		NPaintEngine::ActionMouse* inActionMouse, const ZCursor& inCursor);
	virtual ~MouseTracker_ActionMouseMultiClick();

// From ZMouseTracker
	virtual void ButtonDown(ZPoint inPanePoint, ZMouse::Button inButton);
	virtual void ButtonUp(ZPoint inPanePoint, ZMouse::Button inButton);
	virtual void Moved(ZPoint inPanePoint);
	virtual void Lingered(ZPoint inPanePoint);

	virtual void Installed();
	virtual void Removed();
	virtual void Cancelled();

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);

protected:
	NPaintEngine* fPaintEngine;
	ZDC fTargetDC;
	NPaintEngine::ActionMouse* fActionMouse;
	bigtime_t fLastClickTime;
	ZPoint fLastClickPoint;
	ZCursor fCursor;
	};

NPaintEngine::MouseTracker_ActionMouseMultiClick::MouseTracker_ActionMouseMultiClick(
	NPaintEngine* inPaintEngine, ZPoint inStartPoint, const ZDC& inOriginalDC,
	NPaintEngine::ActionMouse* inActionMouse, const ZCursor& inCursor)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine),
	fActionMouse(inActionMouse),
	fLastClickTime(0),
	fCursor(inCursor)
	{
	fPaintEngine->CommitLastAction();
	fTargetDC = inOriginalDC;
	}

NPaintEngine::MouseTracker_ActionMouseMultiClick::~MouseTracker_ActionMouseMultiClick()
	{
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::ButtonDown(
	ZPoint inPanePoint, ZMouse::Button inButton)
	{
	bool finish = false;
	// Only treat as doubleclick if mouse has not moved TQ 
	if ((ZTicks::sNow() - fLastClickTime <= ZTicks::sDoubleClick())
		&& (abs(int(inPanePoint.h - fLastClickPoint.h)) <= 2)
		&& (abs(int(inPanePoint.v - fLastClickPoint.v)) <= 2))
		finish = true;
	fLastClickTime = ZTicks::sNow();
	fLastClickPoint = inPanePoint;

	ZDC targetDC = fTargetDC;
	ZDCRgn changedRgn = fActionMouse->MouseDown(targetDC, fPaintEngine->FromHost(inPanePoint));
	fPaintEngine->InvalidateRgn(changedRgn);

	if (finish)
		this->Finish();
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::ButtonUp(
	ZPoint inPanePoint, ZMouse::Button inButton)
	{
	ZDC targetDC(fTargetDC);
	ZDCRgn changedRgn = fActionMouse->MouseUp(targetDC, fPaintEngine->FromHost(inPanePoint));
	fPaintEngine->InvalidateRgn(changedRgn);
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::Moved(ZPoint inPanePoint)
	{
	ZDC targetDC(fTargetDC);
	ZDCRgn changedRgn = fActionMouse->Moved(targetDC, fPaintEngine->FromHost(inPanePoint));
	fPaintEngine->InvalidateRgn(changedRgn);
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::Lingered(ZPoint inPanePoint)
	{
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::Installed()
	{
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::Removed()
	{
	ZDC targetDC(fTargetDC);
	ZDCRgn changedRgn = fActionMouse->Finished(targetDC);
	fPaintEngine->InvalidateRgn(changedRgn);
	fPaintEngine->PostAction(fActionMouse);
	fActionMouse = nil;
	}

void NPaintEngine::MouseTracker_ActionMouseMultiClick::Cancelled()
	{
	ZDC targetDC(fTargetDC);
	ZDCRgn changedRgn = fActionMouse->Finished(targetDC);
	fPaintEngine->InvalidateRgn(changedRgn);
	fPaintEngine->PostAction(fActionMouse);
	fActionMouse = nil;
	}

bool NPaintEngine::MouseTracker_ActionMouseMultiClick::GetCursor(
	ZPoint inPanePoint, ZCursor& outCursor)
	{
	outCursor = fCursor;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_SetSelection

class NPaintEngine::Action_SetSelection : public NPaintEngine::ActionInval
	{
public:
	Action_SetSelection(const vector<ZPoint>& inPixelSelectionPoints);
	virtual ~Action_SetSelection();

// From NPaintEngine::ActionInval
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);

	virtual string GetUndoText();
	virtual string GetRedoText();
protected:
	vector<ZPoint> fPixelSelectionPoints;
	ZDCRgn fPriorSelectionRgn;
	};

NPaintEngine::Action_SetSelection::Action_SetSelection(const vector<ZPoint>& inPixelSelectionPoints)
:	fPixelSelectionPoints(inPixelSelectionPoints)
	{}

NPaintEngine::Action_SetSelection::~Action_SetSelection()
	{}

ZDCRgn NPaintEngine::Action_SetSelection::DoReturnRgn(NPaintEngine* inEngine)
	{
	fPriorSelectionRgn = inEngine->GetPixelSelectionRgn();
	return inEngine->SetPixelSelectionPoints(fPixelSelectionPoints);
	}

ZDCRgn NPaintEngine::Action_SetSelection::UndoReturnRgn(NPaintEngine* inEngine)
	{
	return inEngine->SetPixelSelectionRgn(fPriorSelectionRgn);
	}

string NPaintEngine::Action_SetSelection::GetUndoText()
	{
	return "Undo Set Selection";
	}

string NPaintEngine::Action_SetSelection::GetRedoText()
	{
	return "Redo Set Selection";
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_SelectRect

class NPaintEngine::MouseTracker_SelectRect : public ZMouseTracker
	{
public:
	MouseTracker_SelectRect(NPaintEngine* inPaintEngine, ZPoint inStartPoint);
	virtual ~MouseTracker_SelectRect();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);

protected:
	NPaintEngine* fPaintEngine;
	};

NPaintEngine::MouseTracker_SelectRect::MouseTracker_SelectRect(
	NPaintEngine* inPaintEngine, ZPoint inStartPoint)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine)
	{
	fPaintEngine->CommitLastAction();
	ZDCRgn changedRgn = fPaintEngine->SetPixelSelectionRgn(ZDCRgn());
	fPaintEngine->InvalidateRgn(changedRgn);
	}

NPaintEngine::MouseTracker_SelectRect::~MouseTracker_SelectRect()
	{
	}

void NPaintEngine::MouseTracker_SelectRect::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inMoved || inPhase != ePhaseContinue)
		{
		ZRect newRect = ZRect::sMinimalRect(
			fPaintEngine->FromHost(inAnchor), fPaintEngine->FromHost(inNext));
		newRect.bottom -= 1;
		newRect.right -= 1;

		vector<ZPoint> selectedPoints;
		if (inPhase != ePhaseEnd || !newRect.IsEmpty())
			{
			// We're not in phaseEnd, or the newRect is not empty
			selectedPoints.push_back(newRect.TopLeft());
			selectedPoints.push_back(newRect.TopRight());
			selectedPoints.push_back(newRect.BottomRight());
			selectedPoints.push_back(newRect.BottomLeft());
			selectedPoints.push_back(newRect.TopLeft());
			}

		ZDCRgn changedRgn;
		if (inPhase == ePhaseEnd)
			{
			changedRgn |= fPaintEngine->SetPixelSelectionFeedback(vector<ZPoint>());
			if (true)
				{
				// This variant simply sets the selection.
				changedRgn |= fPaintEngine->SetPixelSelectionPoints(selectedPoints);
				}
			else
				{
				// This variant makes changes to the selection an undoable action.
				fPaintEngine->PostAction(new Action_SetSelection(selectedPoints));
				}
			}
		else
			{
			changedRgn |= fPaintEngine->SetPixelSelectionFeedback(selectedPoints);
			}
		fPaintEngine->InvalidateRgn(changedRgn);
		}
	}

bool NPaintEngine::MouseTracker_SelectRect::GetCursor(ZPoint inPanePoint, ZCursor& outCursor)
	{
	return fPaintEngine->GetCursor(NPaintEngine::eCursorNameSelect, outCursor);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_SelectFreeform

class NPaintEngine::MouseTracker_SelectFreeform : public ZMouseTracker
	{
public:
	MouseTracker_SelectFreeform(NPaintEngine* inPaintEngine, ZPoint inStartPoint);
	virtual ~MouseTracker_SelectFreeform();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);
protected:
	NPaintEngine* fPaintEngine;
	vector<ZPoint> fPoints;
	};

NPaintEngine::MouseTracker_SelectFreeform::MouseTracker_SelectFreeform(
	NPaintEngine* inPaintEngine, ZPoint inStartPoint)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine)
	{
	fPaintEngine->CommitLastAction();
	ZDCRgn changedRgn = fPaintEngine->SetPixelSelectionRgn(ZDCRgn());
	fPaintEngine->InvalidateRgn(changedRgn);
	}

NPaintEngine::MouseTracker_SelectFreeform::~MouseTracker_SelectFreeform()
	{}

void NPaintEngine::MouseTracker_SelectFreeform::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inMoved || inPhase != ePhaseContinue)
		{
		if (inNext != inPrevious)
			fPoints.push_back(fPaintEngine->FromHost(inNext));

		ZDCRgn changedRgn;
		if (inPhase == ePhaseEnd)
			{
			changedRgn |= fPaintEngine->SetPixelSelectionFeedback(vector<ZPoint>());

			if (true)
				changedRgn |= fPaintEngine->SetPixelSelectionPoints(fPoints);
			else
				fPaintEngine->PostAction(new Action_SetSelection(fPoints));
			}
		else
			{
			changedRgn |= fPaintEngine->SetPixelSelectionFeedback(fPoints);
			}
		fPaintEngine->InvalidateRgn(changedRgn);
		}
	}

bool NPaintEngine::MouseTracker_SelectFreeform::GetCursor(ZPoint inPanePoint, ZCursor& outCursor)
	{
	return fPaintEngine->GetCursor(NPaintEngine::eCursorNameLasso, outCursor);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Clear

class NPaintEngine::Action_Clear : public NPaintEngine::ActionPaint
	{
public:
	Action_Clear();
	virtual ~Action_Clear();

// From NPaintEngine::ActionPaint
	virtual ZDCRgn CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);

	virtual string GetUndoText();
	virtual string GetRedoText();

protected:
	ZRef<NPaintDataRep> fPriorPaintData;
	};

NPaintEngine::Action_Clear::Action_Clear()
	{}

NPaintEngine::Action_Clear::~Action_Clear()
	{}

ZDCRgn NPaintEngine::Action_Clear::CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_Clear::DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	fPriorPaintData = inEngine->GetSelectionPaintData();
	return inEngine->ReplaceSelectionPaintData(ZRef<NPaintDataRep>());
	}

ZDCRgn NPaintEngine::Action_Clear::UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	return inEngine->ReplaceSelectionPaintData(fPriorPaintData);
	}

ZDCRgn NPaintEngine::Action_Clear::RedoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	return inEngine->ReplaceSelectionPaintData(ZRef<NPaintDataRep>());
	}

string NPaintEngine::Action_Clear::GetUndoText()
	{ return "Undo Clear"; }

string NPaintEngine::Action_Clear::GetRedoText()
	{ return "Redo Clear"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Cut

class NPaintEngine::Action_Cut : public NPaintEngine::ActionInval
	{
public:
	Action_Cut();
	virtual ~Action_Cut();

// From NPaintEngine::ActionInval
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);

	virtual string GetUndoText();
	virtual string GetRedoText();

protected:
	ZRef<NPaintDataRep> fPaintData;
	};

NPaintEngine::Action_Cut::Action_Cut()
	{}

NPaintEngine::Action_Cut::~Action_Cut()
	{}

ZDCRgn NPaintEngine::Action_Cut::CommitReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_Cut::DoReturnRgn(NPaintEngine* inEngine)
	{
	ZTuple theTuple;
	inEngine->GetSelectionForClipboard(theTuple, fPaintData);

	ZClipboard::sSet(theTuple);
	return inEngine->ReplaceSelectionPaintData(ZRef<NPaintDataRep>());
	}

ZDCRgn NPaintEngine::Action_Cut::UndoReturnRgn(NPaintEngine* inEngine)
	{
	return inEngine->ReplaceSelectionPaintData(fPaintData);
	}

ZDCRgn NPaintEngine::Action_Cut::RedoReturnRgn(NPaintEngine* inEngine)
	{
	return inEngine->ReplaceSelectionPaintData(ZRef<NPaintDataRep>());
	}

string NPaintEngine::Action_Cut::GetUndoText()
	{ return "Undo Cut"; }

string NPaintEngine::Action_Cut::GetRedoText()
	{ return "Redo Cut"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Paste

class NPaintEngine::Action_Paste : public NPaintEngine::ActionInval
	{
public:
	Action_Paste();
	virtual ~Action_Paste();

// From NPaintEngine::ActionInval
	virtual ZDCRgn CommitReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn DoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn UndoReturnRgn(NPaintEngine* inEngine);
	virtual ZDCRgn RedoReturnRgn(NPaintEngine* inEngine);

	virtual string GetUndoText();
	virtual string GetRedoText();

protected:
	ZRef<NPaintDataRep> fClipPaintData;
	};

NPaintEngine::Action_Paste::Action_Paste()
	{
	}

NPaintEngine::Action_Paste::~Action_Paste()
	{}

ZDCRgn NPaintEngine::Action_Paste::CommitReturnRgn(NPaintEngine* inEngine)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_Paste::DoReturnRgn(NPaintEngine* inEngine)
	{
	try
		{
		ZTuple theTuple = ZClipboard::sGet();
		if (theTuple.Has("x-zlib/NPaintData"))
			{
			if (ZRef<NPaintDataRep> thePaintData
				= ZRefDynamicCast<NPaintDataRep>(theTuple.GetRefCounted("x-zlib/NPaintData")))
				{
				fClipPaintData = thePaintData;
				}
			}

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
		else if (theTuple.Has("image/x-PICT"))
			{
			ZMemoryBlock theMemoryBlock = theTuple.GetRaw("image/x-PICT");
			if (theMemoryBlock.GetSize())
				{
				ZRef<NPaintDataRep> theRep = new NPaintDataRep;
				theRep->FromStream_PICT(ZStreamRWPos_MemoryBlock(theMemoryBlock));
				fClipPaintData = theRep;
				}
			}
#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#if ZCONFIG(OS, Win32)
		else if (theTuple.Has("image/x-BMP"))
			{
			ZMemoryBlock theMemoryBlock = theTuple.GetRaw("image/x-BMP");
			if (theMemoryBlock.GetSize())
				{
				ZRef<NPaintDataRep> theRep = new NPaintDataRep;
				theRep->FromStream_BMP(ZStreamRWPos_MemoryBlock(theMemoryBlock));
				fClipPaintData = theRep;
				}
			}
#endif // ZCONFIG(OS, Win32)
		else if (theTuple.Has("text/plain"))
			{
			string theText = theTuple.GetString("text/plain");
			ZRef<NPaintDataRep> theRep = new NPaintDataRep(ZRect(inEngine->GetSize()), theText);
			fClipPaintData = theRep;
			}
		}
	catch (...)
		{
		}

	return inEngine->PastePaintData(fClipPaintData);
	}

ZDCRgn NPaintEngine::Action_Paste::UndoReturnRgn(NPaintEngine* inEngine)
	{
	return inEngine->ReplaceSelectionPaintData(ZRef<NPaintDataRep>());
	}

ZDCRgn NPaintEngine::Action_Paste::RedoReturnRgn(NPaintEngine* inEngine)
	{
	return inEngine->ReplaceSelectionPaintData(fClipPaintData);
	}

string NPaintEngine::Action_Paste::GetUndoText()
	{ return "Undo Paste"; }

string NPaintEngine::Action_Paste::GetRedoText()
	{ return "Redo Paste"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_EraseAll

class NPaintEngine::Action_EraseAll : public NPaintEngine::ActionPaint
	{
public:
	Action_EraseAll();
	virtual ~Action_EraseAll();

// From NPaintEngine::ActionPaint
	virtual ZDCRgn CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);

	virtual string GetUndoText();
	virtual string GetRedoText();

protected:
	ZDCPixmap fPixmapUndo;
	};

NPaintEngine::Action_EraseAll::Action_EraseAll()
	{}
NPaintEngine::Action_EraseAll::~Action_EraseAll()
	{}

ZDCRgn NPaintEngine::Action_EraseAll::CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_EraseAll::DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	ZDC localDC(inPixelDC);
	ZDCRgn theClip = localDC.GetClip();
	fPixmapUndo = localDC.GetPixmap(theClip.Bounds());
	localDC.SetInk(ZRGBColor::sWhite);
	localDC.Fill(theClip);
	return theClip;
	}

ZDCRgn NPaintEngine::Action_EraseAll::UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	ZDC localDC(inPixelDC);
	localDC.Draw(ZPoint::sZero, fPixmapUndo);
	return localDC.GetClip();
	}

string NPaintEngine::Action_EraseAll::GetUndoText()
	{ return "Undo Erase All"; }

string NPaintEngine::Action_EraseAll::GetRedoText()
	{ return "Redo Erase All"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_PaintBucket

class NPaintEngine::Action_PaintBucket : public NPaintEngine::ActionPaint
	{
public:
	Action_PaintBucket(ZPoint inSeedLocation, const ZDCInk& inInkFill);
	virtual ~Action_PaintBucket();

// From NPaintEngine::ActionPaint
	virtual ZDCRgn CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);
	virtual ZDCRgn UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC);

	virtual string GetUndoText();
	virtual string GetRedoText();
protected:
	ZPoint fSeedLocation;
	ZDCInk fInkFill;
	ZDCPixmap fPixmapUndo;
	};

NPaintEngine::Action_PaintBucket::Action_PaintBucket(ZPoint inSeedLocation, const ZDCInk& inInkFill)
:	fSeedLocation(inSeedLocation), fInkFill(inInkFill)
	{}
NPaintEngine::Action_PaintBucket::~Action_PaintBucket()
	{}

ZDCRgn NPaintEngine::Action_PaintBucket::CommitWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{ return ZDCRgn(); }

ZDCRgn NPaintEngine::Action_PaintBucket::DoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	ZDC localDC(inPixelDC);
	ZDCRgn theClip = localDC.GetClip();
	fPixmapUndo = localDC.GetPixmap(theClip.Bounds());
	localDC.SetInk(fInkFill);
	localDC.FloodFill(fSeedLocation);
	return theClip;
	}

ZDCRgn NPaintEngine::Action_PaintBucket::UndoWithDC(NPaintEngine* inEngine, const ZDC& inPixelDC)
	{
	ZDC localDC(inPixelDC);
	localDC.Draw(ZPoint::sZero, fPixmapUndo);
	return localDC.GetClip();
	}

string NPaintEngine::Action_PaintBucket::GetUndoText()
	{ return "Undo Paint Bucket"; }

string NPaintEngine::Action_PaintBucket::GetRedoText()
	{ return "Redo Paint Bucket"; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Rectangle

class NPaintEngine::Action_Rectangle : public NPaintEngine::ActionMouse
	{
public:
	Action_Rectangle(const ZDC& inOriginalDC,
		const ZDCInk& inInkFrame, ZCoord inWidthFrame, const ZDCInk& inInkFill);
	virtual ~Action_Rectangle();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);
protected:
	ZDCInk fInkFrame;
	ZCoord fWidthFrame;
	ZDCInk fInkFill;
	};

NPaintEngine::Action_Rectangle::Action_Rectangle(const ZDC& inOriginalDC,
	const ZDCInk& inInkFrame, ZCoord inWidthFrame, const ZDCInk& inInkFill)
:	ActionMouse(inOriginalDC),
	fInkFrame(inInkFrame),
	fWidthFrame(inWidthFrame),
	fInkFill(inInkFill)
	{}
NPaintEngine::Action_Rectangle::~Action_Rectangle()
	{}

string NPaintEngine::Action_Rectangle::GetUndoText()
	{ return "Undo Rectangle"; }

string NPaintEngine::Action_Rectangle::GetRedoText()
	{ return "Redo Rectangle"; }

ZDCRgn NPaintEngine::Action_Rectangle::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	ZRect oldRect = ZRect::sMinimalRect(inAnchor, inPrevious);
	ZRect newRect = ZRect::sMinimalRect(inAnchor, inNext);
	inTargetDC.CopyFrom(oldRect.TopLeft(), fOriginalDC, oldRect);

	inTargetDC.SetInk(fInkFill);
	inTargetDC.Fill(newRect);

	if (!fInkFrame.IsSameAs(fInkFill))
		{
		inTargetDC.SetInk(fInkFrame);
		inTargetDC.SetPenWidth(fWidthFrame);
		inTargetDC.Frame(newRect);
		}

	return ZDCRgn(newRect) | ZDCRgn(oldRect);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Oval

class NPaintEngine::Action_Oval : public NPaintEngine::ActionMouse
	{
public:
	Action_Oval(const ZDC& inOriginalDC,
		const ZDCInk& inInkFrame, ZCoord inWidthFrame, const ZDCInk& inInkFill);
	virtual ~Action_Oval();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);
protected:
	ZDCInk fInkFrame;
	ZCoord fWidthFrame;
	ZDCInk fInkFill;
	};

NPaintEngine::Action_Oval::Action_Oval(const ZDC& inOriginalDC,
	const ZDCInk& inInkFrame, ZCoord inWidthFrame, const ZDCInk& inInkFill)
:	ActionMouse(inOriginalDC),
	fInkFrame(inInkFrame),
	fWidthFrame(inWidthFrame),
	fInkFill(inInkFill)
	{}
NPaintEngine::Action_Oval::~Action_Oval()
	{}

string NPaintEngine::Action_Oval::GetUndoText()
	{
	return "Undo Oval";
	}

string NPaintEngine::Action_Oval::GetRedoText()
	{
	return "Redo Oval";
	}

ZDCRgn NPaintEngine::Action_Oval::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	ZRect oldRect = ZRect::sMinimalRect(inAnchor, inPrevious);
	ZRect newRect = ZRect::sMinimalRect(inAnchor, inNext);

	inTargetDC.CopyFrom(oldRect.TopLeft(), fOriginalDC, oldRect);

	inTargetDC.SetInk(fInkFill);
	inTargetDC.FillEllipse(newRect);

	if (!fInkFrame.IsSameAs(fInkFill))
		{
		inTargetDC.SetInk(fInkFrame);
		inTargetDC.SetPenWidth(fWidthFrame);
		inTargetDC.FrameEllipse(newRect);
		}

	return ZDCRgn(newRect) | ZDCRgn(oldRect);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Line

class NPaintEngine::Action_Line : public NPaintEngine::ActionMouse
	{
public:
	Action_Line(const ZDC& inOriginalDC, const ZDCInk& inInk, const ZDCRgn& inPenRgn);
	virtual ~Action_Line();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);
protected:
	ZDCInk fInk;
	ZDCRgn fPenRgn;
	};

NPaintEngine::Action_Line::Action_Line(const ZDC& inOriginalDC,
	const ZDCInk& inInk, const ZDCRgn& inPenRgn)
:	ActionMouse(inOriginalDC),
	fInk(inInk),
	fPenRgn(inPenRgn)
	{}

NPaintEngine::Action_Line::~Action_Line()
	{}

string NPaintEngine::Action_Line::GetUndoText()
	{
	return "Undo Line";
	}

string NPaintEngine::Action_Line::GetRedoText()
	{
	return "Redo Line";
	}

ZDCRgn NPaintEngine::Action_Line::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	ZRect anchorRect = fPenRgn.Bounds() + inAnchor;
	ZRect oldRect = anchorRect | (fPenRgn.Bounds() + inPrevious);
	ZRect newRect = anchorRect | (fPenRgn.Bounds() + inNext);

	inTargetDC.CopyFrom(oldRect.TopLeft(), fOriginalDC, oldRect);

	inTargetDC.SetInk(fInk);

	ZUtil_Graphics::sDrawFatLine(inTargetDC, fPenRgn, inAnchor, inNext);

	return ZDCRgn(newRect) | ZDCRgn(oldRect);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Brush

class NPaintEngine::Action_Brush : public NPaintEngine::ActionMouse
	{
public:
	Action_Brush(const ZDC& inOriginalDC, const ZDCInk& inInk, const ZDCRgn& inPenRgn);
	virtual ~Action_Brush();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);
protected:
	ZDCInk fInk;
	ZDCRgn fPenRgn;
	};

NPaintEngine::Action_Brush::Action_Brush(const ZDC& inOriginalDC,
	const ZDCInk& inInk, const ZDCRgn& inPenRgn)
:	ActionMouse(inOriginalDC),
	fInk(inInk),
	fPenRgn(inPenRgn)
	{}

NPaintEngine::Action_Brush::~Action_Brush()
	{}

string NPaintEngine::Action_Brush::GetUndoText()
	{
	return "Undo Brush";
	}

string NPaintEngine::Action_Brush::GetRedoText()
	{
	return "Redo Brush";
	}

ZDCRgn NPaintEngine::Action_Brush::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	inTargetDC.SetInk(fInk);
	ZUtil_Graphics::sDrawFatLine(inTargetDC, fPenRgn, inPrevious, inNext);

	ZRect previousRect = fPenRgn.Bounds() + inPrevious;
	ZRect nextRect = fPenRgn.Bounds() + inNext;

	return previousRect | nextRect;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Eraser

class NPaintEngine::Action_Eraser : public NPaintEngine::ActionMouse
	{
public:
	Action_Eraser(const ZDC& inOriginalDC, const ZDCInk& inInk, const ZDCRgn& inRgn);
	virtual ~Action_Eraser();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn DoIt(ZDC& inTargetDC, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext);
protected:
	ZDCInk fInk;
	ZDCRgn fRgn;
	};

NPaintEngine::Action_Eraser::Action_Eraser(const ZDC& inOriginalDC,
	const ZDCInk& inInk, const ZDCRgn& inRgn)
:	ActionMouse(inOriginalDC),
	fInk(inInk),
	fRgn(inRgn)
	{}

NPaintEngine::Action_Eraser::~Action_Eraser()
	{}

string NPaintEngine::Action_Eraser::GetUndoText()
	{
	return "Undo Eraser";
	}

string NPaintEngine::Action_Eraser::GetRedoText()
	{
	return "Redo Eraser";
	}

ZDCRgn NPaintEngine::Action_Eraser::DoIt(ZDC& inTargetDC,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext)
	{
	inTargetDC.SetInk(fInk);
	ZUtil_Graphics::sDrawFatLine(inTargetDC, fRgn, inPrevious, inNext);

	ZRect previousRect = fRgn.Bounds() + inPrevious;
	ZRect nextRect = fRgn.Bounds() + inNext;

	return previousRect | nextRect;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::Action_Polygon

class NPaintEngine::Action_Polygon : public NPaintEngine::ActionMouse
	{
public:
	Action_Polygon(const ZDC& inOriginalDC, ZPoint inEngineHitPoint,
		const ZDCInk& inInkFrame, ZDCRgn inPenRgn, const ZDCInk& inInkFill);

	virtual ~Action_Polygon();

	virtual string GetUndoText();
	virtual string GetRedoText();

// From NPaintEngine::ActionMouse
	virtual ZDCRgn MouseDown(ZDC& inTargetDC, ZPoint inLocation);
	virtual ZDCRgn Moved(ZDC& inTargetDC, ZPoint inLocation);
	virtual ZDCRgn Finished(ZDC& inTargetDC);

protected:
	vector<ZPoint> fPoints;

	ZPoint fLastAnchorPoint;
	ZPoint fLastEndPoint;

	ZDC fInterimDC;
	ZDCInk fInkFrame;
	ZDCRgn fPenRgn;
	ZDCInk fInkFill;
	bool fClosePolygon;
	};

NPaintEngine::Action_Polygon::Action_Polygon(const ZDC& inOriginalDC, ZPoint inEngineHitPoint,
	const ZDCInk& inInkFrame, ZDCRgn inPenRgn, const ZDCInk& inInkFill)
:	ActionMouse(inOriginalDC),
	fLastAnchorPoint(inEngineHitPoint),
	fLastEndPoint(inEngineHitPoint),
	fInkFrame(inInkFrame),
	fPenRgn(inPenRgn),
	fInkFill(inInkFill)
	{
	fClosePolygon = true;
	// Create the DC which holds the result of applying all line
	// segments *prior* to the one currently being tracked
	fInterimDC = ZDC_Off(inOriginalDC, true);

	// Copy the snapshot pixels across
	ZRect sourceRect(inOriginalDC.GetClip().Bounds());
	fInterimDC.CopyFrom(sourceRect.TopLeft(), inOriginalDC, sourceRect);

	fPoints.push_back(inEngineHitPoint);
	}

NPaintEngine::Action_Polygon::~Action_Polygon()
	{}

string NPaintEngine::Action_Polygon::GetUndoText()
	{
	return "Undo Polygon";
	}

string NPaintEngine::Action_Polygon::GetRedoText()
	{
	return "Redo Polygon";
	}

ZDCRgn NPaintEngine::Action_Polygon::MouseDown(ZDC& inTargetDC, ZPoint inLocation)
	{
	ZRect anchorRect = fPenRgn.Bounds() + fLastAnchorPoint;
	ZRect oldRect = anchorRect | (fPenRgn.Bounds() + fLastEndPoint);
	ZRect newRect = anchorRect | (fPenRgn.Bounds() + inLocation);
	ZRect unionRect = oldRect | newRect;
	fInterimDC.SetInk(fInkFrame);
	ZUtil_Graphics::sDrawFatLine(fInterimDC, fPenRgn, fLastAnchorPoint, inLocation);
	inTargetDC.CopyFrom(unionRect.TopLeft(), fInterimDC, unionRect);
	fLastAnchorPoint = inLocation;
	fLastEndPoint = inLocation;
	fPoints.push_back(inLocation);

	return ZDCRgn(newRect) | ZDCRgn(oldRect);
	}

ZDCRgn NPaintEngine::Action_Polygon::Moved(ZDC& inTargetDC, ZPoint inLocation)
	{
	ZRect anchorRect = fPenRgn.Bounds() + fLastAnchorPoint;
	ZRect oldRect = anchorRect | (fPenRgn.Bounds() + fLastEndPoint);
	ZRect newRect = anchorRect | (fPenRgn.Bounds() + inLocation);

	inTargetDC.CopyFrom(oldRect.TopLeft(), fInterimDC, oldRect);

	inTargetDC.SetInk(fInkFrame);
	ZUtil_Graphics::sDrawFatLine(inTargetDC, fPenRgn, fLastAnchorPoint, inLocation);

	fLastEndPoint = inLocation;
	return ZDCRgn(newRect) | ZDCRgn(oldRect);
	}

static void sDrawPointPair(ZDC& inDC, const ZDCRgn& inPenRgn,
	ZPoint inPoint1, ZPoint inPoint2, ZDCRgn& ioRgn)
	{
	ZRect rect1 = inPenRgn.Bounds() + inPoint1;
	ZRect rect2 = inPenRgn.Bounds() + inPoint2;
	ZUtil_Graphics::sDrawFatLine(inDC, inPenRgn, inPoint1, inPoint2);
	ioRgn |= rect1 | rect2;
	}

ZDCRgn NPaintEngine::Action_Polygon::Finished(ZDC& inTargetDC)
	{
	inTargetDC.CopyFrom(ZPoint::sZero, fOriginalDC, fOriginalDC.GetClip().Bounds());

	ZDCRgn theRgn;
	if (fPoints.size() > 1 && fInkFill)
		{
		theRgn = ZDCRgn::sPoly(&fPoints[0], fPoints.size());
		inTargetDC.SetInk(fInkFill);
		inTargetDC.Fill(theRgn);
		}

	if (fInkFrame)
		{
		inTargetDC.SetInk(fInkFrame);
		for (size_t x = 1; x < fPoints.size(); ++x)
			::sDrawPointPair(inTargetDC, fPenRgn, fPoints[x-1], fPoints[x], theRgn);
		if (fClosePolygon && fPoints.size() > 2)
			::sDrawPointPair(inTargetDC, fPenRgn, fPoints.back(), fPoints.front(), theRgn);
		}

	ZRect rect1 = fPenRgn.Bounds() + fLastEndPoint;
	ZRect rect2 = fPenRgn.Bounds() + fLastAnchorPoint;
	theRgn |= (rect1 | rect2);

	return theRgn;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::MouseTracker_Dropper

class NPaintEngine::MouseTracker_Dropper : public ZMouseTracker
	{
public:
	MouseTracker_Dropper(NPaintEngine* inPaintEngine, ZPoint inStartPoint, const ZDC& inPixelDC);
	virtual ~MouseTracker_Dropper();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase,
		ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);

protected:
	NPaintEngine* fPaintEngine;
	ZDC fPixelDC;
	};

NPaintEngine::MouseTracker_Dropper::MouseTracker_Dropper(NPaintEngine* inPaintEngine,
	ZPoint inStartPoint, const ZDC& inPixelDC)
:	ZMouseTracker(inPaintEngine->GetHost()->GetPaintEnginePane(), inStartPoint),
	fPaintEngine(inPaintEngine), fPixelDC(inPixelDC)
	{
	}

NPaintEngine::MouseTracker_Dropper::~MouseTracker_Dropper()
	{
	}

void NPaintEngine::MouseTracker_Dropper::TrackIt(Phase inPhase,
	ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZRGBColor theColor = fPixelDC.GetPixel(fPaintEngine->FromHost(inNext));
	fPaintEngine->GetPaintState()->SetColor_Stroke(theColor);
	}

bool NPaintEngine::MouseTracker_Dropper::GetCursor(ZPoint inPanePoint, ZCursor& outCursor)
	{
	return fPaintEngine->GetCursor(NPaintEngine::eCursorNameDropper, outCursor);
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine

// This is a 11..11.. marching ants pattern.
//static ZDCPattern sSelectionPattern = { { 0xCC, 0x66, 0x33, 0x99, 0xCC, 0x66, 0x33, 0x99 } };

// The one we use is a 11111... pattern, which is more visible.
static ZDCPattern sSelectionPattern = { { 0xF8, 0x7C, 0x3E, 0x1F, 0x8F, 0xC7, 0xE3, 0xF1 } };

NPaintEngine::NPaintEngine(Host* inHost, const ZRef<PaintState>& inPaintState, ZPoint inSize)
	{
	fChangeCount = 0;

	fLastKnownTool = eToolTypeArrow;
	fLastKnownToolValid = false;

	fTimeOfLastTick = 0;

	fHost = inHost;
	fPaintState = inPaintState;
	fPaintState->GetInterest_Tool()->RegisterClient(this);
	fPaintState->GetInterest_TextColor()->RegisterClient(this);
	fPixelSelectionPattern = sSelectionPattern;
	fLastAction = nil;
	fLastActionDone = false;

	fSize = inSize;

	fPixelDC = ZDC_Off(ZDCScratch::sGet(), fSize, ZDCPixmapNS::eFormatEfficient_Color_32);
	fPixelDC.SetInk(ZRGBColor::sWhite);
	fPixelDC.Fill(ZRect(inSize));

	fTextPlane = new TextPlane(this);
	}

NPaintEngine::NPaintEngine(Host* inHost,
	const ZRef<PaintState>& inPaintState, const ZRef<NPaintDataRep>& inPaintData)
	{
	fChangeCount = 0;

	fLastKnownTool = eToolTypeArrow;
	fLastKnownToolValid = false;

	fTimeOfLastTick = 0;

	fHost = inHost;
	fPaintState = inPaintState;
	fPixelSelectionPattern = sSelectionPattern;

	fLastAction = nil;
	fLastActionDone = false;

	try
		{
		if (inPaintData)
			{
			fSize = inPaintData->fPixmap.Size();
			if (fSize.h <= 0)
				fSize.h = 4;
			if (fSize.v <= 0)
				fSize.v = 4;

			fPixelDC = ZDC_Off(ZDCScratch::sGet(), fSize, ZDCPixmapNS::eFormatEfficient_Color_32);
			fPixelDC.Draw(ZPoint::sZero, inPaintData->fPixmap);
			fTextPlane = new TextPlane(this,
				inPaintData->fStrings, inPaintData->fFonts,
				inPaintData->fBounds, inPaintData->fColors);
			}
		else
			{
			fSize.h = fSize.v = 4;
			fPixelDC = ZDC_Off(ZDCScratch::sGet(), fSize, ZDCPixmapNS::eFormatEfficient_Color_32);
			fTextPlane = new TextPlane(this);
			}
		}
	catch (...)
		{
		ZDebugStop(0);
		}

	// Attach to the interest as the last thing we do, in case we throw
	// an exception whilst being created.
	fPaintState->GetInterest_Tool()->RegisterClient(this);
	fPaintState->GetInterest_TextColor()->RegisterClient(this);
	}

NPaintEngine::~NPaintEngine()
	{
	fPaintState->GetInterest_Tool()->UnregisterClient(this);
	fPaintState->GetInterest_TextColor()->UnregisterClient(this);
	delete fTextPlane;
	}

void NPaintEngine::InterestChanged(ZInterest* inInterest)
	{
	if (inInterest == fPaintState->GetInterest_Tool())
		{
		ZLocker locker(fHost->GetPaintEnginePane()->GetLock());
		fTextPlane->InvalidateAllTextBoxes();

		fLastKnownToolValid = false;
		fTextPlane->InvalidateAllTextBoxes();
		}
	else if (inInterest == fPaintState->GetInterest_TextColor())
		{
		ZLocker locker(fHost->GetPaintEnginePane()->GetLock());
		fTextPlane->UpdateSelectedTextColor(fPaintState->GetColor_Text());
		}
	}

NPaintEngine::ToolType NPaintEngine::GetTool()
	{
	if (!fLastKnownToolValid)
		{
		fLastKnownToolValid = true;
		fLastKnownTool = fPaintState->GetTool();
		}
	return fLastKnownTool;
	}

long NPaintEngine::GetChangeCount()
	{ return fChangeCount + fTextPlane->GetChangeCount(); }

NPaintEngine::Host* NPaintEngine::GetHost()
	{ return fHost; }

ZPoint NPaintEngine::GetSize()
	{
	return fSize;
	}

void NPaintEngine::Draw(const ZDC& inDC, ZPoint inLocation)
	{
	ZDC localDC = inDC;
	localDC.PenNormal();
	localDC.OffsetOrigin(inLocation);
	localDC.OffsetPatternOrigin(inLocation);
	ZDCRgn drawClip = localDC.GetClip() & ZRect(fSize);
	localDC.SetClip(drawClip);
	if (fPixelSelectionPixmap)
		{
		ZDCRgn selectionRgn = fPixelSelectionRgn + fPixelSelectionOffset;
		localDC.SetClip(drawClip & selectionRgn);
		localDC.Draw(fPixelSelectionOffset, fPixelSelectionPixmap);
		if (fHost->GetPaintEngineActive())
			{
			localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, fPixelSelectionPattern));
			localDC.Frame(selectionRgn);
			}
		localDC.SetClip(drawClip - selectionRgn);
		localDC.CopyFrom(drawClip.Bounds().TopLeft(), fPixelDC, drawClip.Bounds());
		localDC.SetClip(drawClip);
		}
	else
		{
		localDC.CopyFrom(drawClip.Bounds().TopLeft(), fPixelDC, drawClip.Bounds());
		}

	if (fHost->GetPaintEngineActive())
		{
		localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, fPixelSelectionPattern));
		for (size_t x = 1; x < fPixelSelectionFeedback.size(); ++x)
			localDC.Line(fPixelSelectionFeedback[x-1], fPixelSelectionFeedback[x]);
		}

	fTextPlane->Draw(localDC);
	}

bool NPaintEngine::Click(ZPoint inHostHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZPoint theEngineHitPoint = this->FromHost(inHostHitPoint);

	ToolType theToolType = this->GetTool();

	// Arrow & text tools get handled by text objects
	if (theToolType == eToolTypeArrow || theToolType == eToolTypeText)
		{
		fTextPlane->Click(theEngineHitPoint, inEvent);
		return true;
		}
	else if (theToolType == eToolTypeText)
		{
		fTextPlane->Click(theEngineHitPoint, inEvent);
// Create a new text object and start tracking it
//		Text* newText = new Text(this);
//		fVector_Text.insert(fVector_Text.begin(), newText);
//		this->InvalidateRgn(newText->GetRgn(true));
		return true;
		}

	ZDCInk theInk_Stroke = fPaintState->GetInk_Stroke();
	ZDCInk theInk_Fill = fPaintState->GetInk_Fill();
	ZDCRgn theRgn_Pen = fPaintState->GetRgn_Pen();
	ZDCRgn theRgn_Eraser = fPaintState->GetRgn_Eraser();
	ZCoord theWidth_Frame = fPaintState->GetWidth_Frame();

	ActionMouse* theActionMouse = nil;
	ZCursor theCursor(ZCursor::eCursorTypeArrow);

	// Simple mouse actions
	if (theToolType == eToolTypeBrush)
		{
		// theCursor = sCursorFromRgn(theRgn_Pen);
		this->GetCursor(eCursorNamePaintBrush, theCursor);
		theActionMouse = new Action_Brush(fPixelDC, theInk_Stroke, theRgn_Pen);
		}
	else if (theToolType == eToolTypeLine)
		{
		// theCursor = sCursorFromRgn(theRgn_Pen);
		this->GetCursor(eCursorNamePaintBrush, theCursor);
		theActionMouse = new Action_Line(fPixelDC, theInk_Stroke, theRgn_Pen);
		}
	else if (theToolType == eToolTypeEraser)
		{
		this->GetCursor(eCursorNameEraser, theCursor);
		theActionMouse = new Action_Eraser(fPixelDC, ZRGBColor::sWhite, theRgn_Eraser);
		}
	else if (theToolType == eToolTypeRect)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Rectangle(fPixelDC, theInk_Stroke, theWidth_Frame, theInk_Fill);
		}
	else if (theToolType == eToolTypeRectFrame)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Rectangle(fPixelDC, theInk_Stroke, theWidth_Frame, ZDCInk());
		}
	else if (theToolType == eToolTypeOval)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Oval(fPixelDC, theInk_Stroke, theWidth_Frame, theInk_Fill);
		}
	else if (theToolType == eToolTypeOvalFrame)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Oval(fPixelDC, theInk_Stroke, theWidth_Frame, ZDCInk());
		}
	else if (theToolType == eToolTypeFreeform)
		{
		}

	if (theActionMouse)
		{
		ZMouseTracker* theTracker = new MouseTracker_ActionMouse(this,
			inHostHitPoint, fPixelDC, theActionMouse, theCursor);
		theTracker->Install();
		return true;
		}

	// Multi click mouse actions
	if (theToolType == eToolTypePolygon)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Polygon(fPixelDC,
			theEngineHitPoint, theInk_Stroke, theRgn_Pen, theInk_Fill);
		}
	else if (theToolType == eToolTypePolygonFrame)
		{
		this->GetCursor(eCursorNameCrossHairs, theCursor);
		theActionMouse = new Action_Polygon(fPixelDC,
			theEngineHitPoint, theInk_Stroke, theRgn_Pen, ZDCInk());
		}

	if (theActionMouse)
		{
		ZMouseTracker* theTracker = new MouseTracker_ActionMouseMultiClick(this,
			inHostHitPoint, fPixelDC, theActionMouse, theCursor);

		theTracker->Install();
		return true;
		}

	// The droppper
	if (theToolType == eToolTypeDropper)
		{
		ZMouseTracker* theTracker = new MouseTracker_Dropper(this, inHostHitPoint, fPixelDC);
		theTracker->Install();
		return true;
		}

	// Selection
	if (theToolType == eToolTypeSelect || theToolType == eToolTypeLasso)
		{
		ZDCRgn selectionRgn = fPixelSelectionRgn + fPixelSelectionOffset;
		if (selectionRgn.Contains(theEngineHitPoint))
			{
			ZMouseTracker* theTracker = new MouseTracker_SelectionMove(this,
				inHostHitPoint, inEvent.GetOption());

			theTracker->Install();
			return true;
			}
		else if (theToolType == eToolTypeSelect)
			{
			ZMouseTracker* theTracker = new MouseTracker_SelectRect(this, inHostHitPoint);
			theTracker->Install();
			return true;
			}
		else if (theToolType == eToolTypeLasso)
			{
			ZMouseTracker* theTracker = new MouseTracker_SelectFreeform(this, inHostHitPoint);
			theTracker->Install();
			return true;
			}
		}

	if (theToolType == eToolTypeFill)
		{
		this->PostAction(new Action_PaintBucket(theEngineHitPoint, theInk_Fill));
		return true;
		}

	// Didn't recognize the tool
	return true;
	}

bool NPaintEngine::KeyDown(const ZEvent_Key& inEvent)
	{
	if (this->GetTextPlaneCanUseTextTool())
		{
		fTextPlane->KeyDown(inEvent);
		return true;
		}

	if (this->HasSelection())
		{
		unsigned char theCharCode = inEvent.GetCharCode();
		if (theCharCode == ZKeyboard::ccBackspace)
			{
			this->PostAction(new Action_Clear);
			}
		else
			{
			ZPoint delta = ZPoint::sZero;
			switch (theCharCode)
				{
				case ZKeyboard::ccLeft: delta.h = -1; break;
				case ZKeyboard::ccRight: delta.h = 1; break;
				case ZKeyboard::ccUp: delta.v = -1; break;
				case ZKeyboard::ccDown: delta.v = 1; break;
				}

			if (delta != ZPoint::sZero)
				{
				// With the shift key down we move further
				if (inEvent.GetShift())
					delta *= 10;

				// If our last done action is a selection move, then this keystroke
				// will just augment that command's delta.
				bool gotIt = false;
				if (fLastActionDone)
					{
					if (Action_SelectionMove* theAction
						= dynamic_cast<Action_SelectionMove*>(fLastAction))
						{
						gotIt = true;
						theAction->SetDelta(theAction->GetDelta() + delta);
						this->InvalidateRgn(this->OffsetSelection(delta));
						}
					}

				// Otherwise we create a new selection move action.
				if (!gotIt)
					this->PostAction(new Action_SelectionMove(delta, true));
				}
			}
		return true;
		}
	return false; 
	}

void NPaintEngine::Idle(const ZDC& inDC, ZPoint inLocation)
	{
	if (fHost->GetPaintEngineActive())
		{
		if (fPixelSelectionPixmap)
			{
			// also need to check if we are moving the selection or not
			bigtime_t currentTicks = ZTicks::sNow();
			if ((currentTicks - fTimeOfLastTick) >= ZTicks::sPerSecond() / 12)
				{
				ZDCPattern oldAnts = fPixelSelectionPattern;
				ZUtil_Graphics::sMarchAnts(fPixelSelectionPattern);
				ZDCPattern deltaPattern;
				bool patternChanged;
				ZUtil_Graphics::sCalculatePatternDifference(oldAnts, fPixelSelectionPattern,
					&deltaPattern, &patternChanged);

				if (patternChanged)
					{
					ZDC localDC = inDC;
					localDC.OffsetOrigin(inLocation);
					localDC.OffsetPatternOrigin(inLocation);
					localDC.SetClip(localDC.GetClip() & ZRect(fSize));
					localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, deltaPattern));
					localDC.SetPenWidth(1);
					localDC.SetMode(ZDC::modeXor);
					localDC.Frame(fPixelSelectionRgn + fPixelSelectionOffset);
					}
				fTimeOfLastTick = currentTicks;
				}
			}
		if (this->GetTextPlaneActive())
			fTextPlane->Idle(inDC, inLocation);
		}
	}

void NPaintEngine::Activate(bool inIsNowActive)
	{
	if (fPixelSelectionRgn)
		this->InvalidateRgn(fPixelSelectionRgn + fPixelSelectionOffset);
	fTextPlane->Activate(inIsNowActive);
	}

void NPaintEngine::SetupMenus(ZMenuSetup* inMenuSetup)
	{
	ZRef<ZMenuItem> theUndoItem = inMenuSetup->GetMenuItem(mcUndo);
	if (theUndoItem)
		{
		if (fLastAction)
			{
			if (fLastActionDone)
				theUndoItem->SetText(fLastAction->GetUndoText());
			else
				theUndoItem->SetText(fLastAction->GetRedoText());
			theUndoItem->SetEnabled(true);
			}
		}
	if (this->HasSelection())
		{
#if 1 // expedient. -ec 99.10.28
		inMenuSetup->SetItemText(mcCopy, "Copy Selection");
		inMenuSetup->SetItemText(mcCut, "Cut Selection");
		inMenuSetup->SetItemText(mcClear, "Clear Selection");
#else // proper
		inMenuSetup->SetItemText(mcCopy, ZString::sFromStrResource(kRSRC_KBViewWindow_CopyPicture));
		inMenuSetup->SetItemText(mcCut, ZString::sFromStrResource(kRSRC_KBViewWindow_CutPicture));
		inMenuSetup->SetItemText(mcClear, ZString::sFromStrResource(kRSRC_KBViewWindow_ClearPicture));
#endif
		inMenuSetup->EnableItem(mcClear);
		inMenuSetup->EnableItem(mcCut);
		inMenuSetup->EnableItem(mcCopy);
		}

	if (ZClipboard::sGet().Has("x-zlib/NPaintData") ||
		ZClipboard::sGet().Has("image/x-PICT") ||
		ZClipboard::sGet().Has("image/x-BMP"))
		{
// this is not great, but we are already using "Paste Picture" in KBViewWindow. -ec 99.11.15
//		inMenuSetup->SetItemText(mcPaste, "Paste Picture");
		inMenuSetup->SetItemText(mcPaste, "Paste");

		inMenuSetup->EnableItem(mcPaste);
		}
	else if (ZClipboard::sGet().Has("text/plain"))
		{
		inMenuSetup->SetItemText(mcPaste, "Paste Text");
		inMenuSetup->EnableItem(mcPaste);
		}

	if (this->GetTextPlaneActive())
		fTextPlane->SetupMenus(inMenuSetup);
	}

bool NPaintEngine::MenuMessage(const ZMessage& inMessage)
	{
	if (this->GetTextPlaneActive())
		{
		if (fTextPlane->MenuMessage(inMessage))
			return true;
		}

	switch (inMessage.GetInt32("menuCommand"))
		{
		case mcUndo:
			{
			if (fLastAction)
				{
				if (fLastActionDone)
					{
					try
						{
						fLastAction->Undo(this);
						fLastActionDone = false;
						--fChangeCount;
						}
					catch (...)
						{}
					}
				else
					{
					try
						{
						fLastAction->Redo(this);
						fLastActionDone = true;
						++fChangeCount;
						}
					catch (...)
						{}
					}
				return true;
				}
			}
			break;
		case mcClear:
			{
			if (this->HasSelection())
				{
				this->PostAction(new Action_Clear);
				return true;
				}
			}
			break;
		case mcCut:
			{
			if (this->HasSelection())
				{
				this->PostAction(new Action_Cut);
				return true;
				}
			}
			break;
		case mcCopy:
			{
			if (this->HasSelection())
				{
				ZTuple theTuple;
				ZRef<NPaintDataRep> thePaintData;
				this->GetSelectionForClipboard(theTuple, thePaintData);
				ZClipboard::sSet(theTuple);
				return true;
				}
			}
			break;
		case mcPaste:
			{
			ZTuple theTuple = ZClipboard::sGet();
			if (theTuple.Has("x-zlib/NPaintData") ||
				theTuple.Has("text/plain") ||
				theTuple.Has("image/x-PICT") ||
				theTuple.Has("image/x-BMP"))
				{
				this->PostAction(new Action_Paste);
				return true;
				}
			}
			break;
		}
	return false;
	}

void NPaintEngine::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	if (inMotion != ZMouse::eMotionLinger)
		fHost->InvalidatePaintEngineCursor();
	}

bool NPaintEngine::GetCursor(ZPoint inHostLocation, ZCursor& outCursor)
	{
	// Need to turn the location and selected tool into a cursor name
	NPaintEngine::ToolType currentTool = this->GetTool();

	if (currentTool == eToolTypeSelect || currentTool == eToolTypeLasso)
		{
		ZPoint theEngineHitPoint = this->FromHost(inHostLocation);
		if (fPixelSelectionRgn.Contains(theEngineHitPoint - fPixelSelectionOffset))
			return fPaintState->GetCursor(eCursorNameDragSelection, outCursor);
		if (currentTool == eToolTypeSelect)
			return fPaintState->GetCursor(eCursorNameSelect, outCursor);
		if (currentTool == eToolTypeLasso)
			return fPaintState->GetCursor(eCursorNameLasso, outCursor);
		}

	CursorName theCursorName = eCursorNameUnknown;
	switch (currentTool)
		{
		case eToolTypeArrow: theCursorName = eCursorNameArrow; break;
		case eToolTypeText: theCursorName = eCursorNameText; break;
		case eToolTypeFill: theCursorName = eCursorNameFill; break;
		case eToolTypeDropper: theCursorName = eCursorNameDropper; break;
		case eToolTypeEraser: theCursorName = eCursorNameEraser; break;
		case eToolTypeSelect: theCursorName = eCursorNameSelect; break;
		case eToolTypeLasso: theCursorName = eCursorNameLasso; break;
		case eToolTypeRect:
		case eToolTypeOval:
		case eToolTypePolygon:
		case eToolTypeRectFrame:
		case eToolTypeOvalFrame:
		case eToolTypePolygonFrame:
		case eToolTypeFreeform:
			theCursorName = eCursorNameCrossHairs;
			break;
		}

	if (theCursorName != eCursorNameUnknown)
		return fPaintState->GetCursor(theCursorName, outCursor);

	// It must be a brush or a line, so build the shaped cursor
	//	outCursor = sCursorFromRgn(fPaintState->GetRgn_Pen());
	//	return true;
	return this->GetCursor(eCursorNamePaintBrush, outCursor);
	}

void NPaintEngine::EraseAll()
	{
	this->PostAction(new Action_EraseAll);
	}

ZPoint NPaintEngine::ToHost(ZPoint inEnginePoint)
	{ return inEnginePoint + fHost->GetPaintEngineLocation(); }
ZPoint NPaintEngine::FromHost(ZPoint inHostPoint)
	{ return inHostPoint - fHost->GetPaintEngineLocation(); }

ZRect NPaintEngine::ToHost(const ZRect& inEngineRect)
	{ return inEngineRect + this->ToHost(ZPoint::sZero); }
ZRect NPaintEngine::FromHost(const ZRect& inHostRect)
	{ return inHostRect - this->ToHost(ZPoint::sZero); }

void NPaintEngine::InvalidateRgn(const ZDCRgn& inEngineRgn)
	{
	fHost->InvalidatePaintEngineRgn(inEngineRgn);
	}

ZRef<NPaintDataRep> NPaintEngine::GetPaintData()
	{
	return this->GetPaintData(ZRect(fSize));
	}

ZRef<NPaintDataRep> NPaintEngine::GetPaintData(const ZRect& iBounds)
	{
	this->CommitLastAction();

	ZDCRgn changedRgn = this->PunchPixelSelectionIntoPixelDC();

	ZRef<NPaintDataRep> theRep = new NPaintDataRep;
	theRep->fRegion = iBounds;
	theRep->fOffset = ZPoint::sZero;
	theRep->fPixmap = fPixelDC.GetPixmap(iBounds);

	vector<string> theStrings;
	vector<ZDCFont> theFonts;
	vector<ZRect> theBounds;
	vector<ZRGBColor> theColors;
	fTextPlane->GetData(theStrings, theFonts, theBounds, theColors);

	// Transcribe text blocks whose bounds intersect iBounds.
	for (size_t x = 0; x < theBounds.size(); ++x)
		{
		if ((theBounds[x] & iBounds).IsEmpty())
			continue;

		theRep->fStrings.push_back(theStrings[x]);
		theRep->fFonts.push_back(theFonts[x]);
		theRep->fBounds.push_back(theBounds[x]);
		theRep->fColors.push_back(theColors[x]);
		}

	return theRep;
	}

void NPaintEngine::GetSelectionForClipboard(ZTuple& outTuple, ZRef<NPaintDataRep>& outPaintData)
	{
	outPaintData = nil;
	outTuple = ZTuple();

	vector<string> theStrings;
	vector<ZDCFont> theFonts;
	vector<ZRect> theBounds;
	vector<ZRGBColor> theColors;

	fTextPlane->GetSelectedData(theStrings, theFonts, theBounds, theColors);

	if (theStrings.size() == 1)
		outTuple.SetString("text/plain", theStrings[0]);

	if (fPixelSelectionRgn || theStrings.size())
		{
		outPaintData = new NPaintDataRep;
		outPaintData->fRegion = fPixelSelectionRgn;
		outPaintData->fOffset = fPixelSelectionOffset;
		outPaintData->fPixmap = fPixelSelectionPixmap;
		outPaintData->fStrings = theStrings;
		outPaintData->fFonts = theFonts;
		outPaintData->fBounds = theBounds;
		outPaintData->fColors = theColors;

		outTuple.SetRefCounted("x-zlib/NPaintData", outPaintData.GetObject());

		#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
			ZMemoryBlock theMemoryBlock;
			outPaintData->ToStream_PICT(ZStreamRWPos_MemoryBlock(theMemoryBlock));
			outTuple.SetRaw("image/x-PICT", theMemoryBlock);
		#endif

		#if ZCONFIG(OS, Win32)
			if (fPixelSelectionRgn)
				{
				// AG 2000-07-31. If we don't have any pixels there's no point creating
				// a BMP -- that format can only represent pixels. (I guess we could do
				// with implementing a ZDC_MetaFile and ToStream_MetaFile.)
				ZMemoryBlock theMemoryBlock;
				outPaintData->ToStream_BMP(ZStreamRWPos_MemoryBlock(theMemoryBlock));
				outTuple.SetRaw("image/x-BMP", theMemoryBlock);
				}
		#endif
		}
	}
ZRef<NPaintDataRep> NPaintEngine::GetSelectionPaintData()
	{
	ZRef<NPaintDataRep> theRep = new NPaintDataRep;
	if (fPixelSelectionRgn)
		{
		theRep->fRegion = fPixelSelectionRgn;
		theRep->fOffset = fPixelSelectionOffset;
		theRep->fPixmap = fPixelSelectionPixmap;
		}
	fTextPlane->GetSelectedData(theRep->fStrings, theRep->fFonts, theRep->fBounds,theRep->fColors);
	return theRep;
	}

ZDCRgn NPaintEngine::ReplaceSelectionPaintData(ZRef<NPaintDataRep> inPaintData)
	{
	ZDCRgn changedRgn = fPixelSelectionRgn + fPixelSelectionOffset;
	if (inPaintData)
		{
		fPixelSelectionPixmap = inPaintData->fPixmap;
		fPixelSelectionRgn = inPaintData->fRegion;
		fPixelSelectionOffset = inPaintData->fOffset;
		}
	else
		{
		fPixelSelectionPixmap = ZDCPixmap();
		fPixelSelectionRgn = ZDCRgn();
		fPixelSelectionOffset = ZPoint::sZero;
		}

	changedRgn |= fPixelSelectionRgn + fPixelSelectionOffset;

	if (inPaintData)
		{
		changedRgn |= fTextPlane->ReplaceSelection(
			inPaintData->fStrings, inPaintData->fFonts,
			inPaintData->fBounds, inPaintData->fColors);
		}
	else
		{
		// Check and see if one of the tools that can deal with text
		// is being used before doing anything - jpj 2000.08.16
		if ((this->GetTool() == eToolTypeText) || (this->GetTool() == eToolTypeArrow))
			{
			changedRgn |= fTextPlane->ReplaceSelection(
				vector<string>(), vector<ZDCFont>(), vector<ZRect>(), vector<ZRGBColor>());
			}
		}

	return changedRgn;
	}

ZDCRgn NPaintEngine::PastePaintData(ZRef<NPaintDataRep> inPaintData)
	{
	// First punch the existing pixel selection into our DC
	ZDCRgn pixelRgn = fPixelSelectionRgn + fPixelSelectionOffset;
	if (fPixelSelectionPixmap)
		{
		ZDC localDC(fPixelDC);
		ZDCRgn drawClip = localDC.GetClip();
		localDC.SetClip(drawClip & pixelRgn);
		localDC.Draw(fPixelSelectionOffset, fPixelSelectionPixmap);
		localDC.SetClip(drawClip);
		}

	ZDCRgn changedRgn = pixelRgn;
	if (inPaintData)
		{
		fPixelSelectionPixmap = inPaintData->fPixmap;
		fPixelSelectionRgn = inPaintData->fRegion;
		fPixelSelectionOffset = inPaintData->fOffset;
		changedRgn |= fPixelSelectionRgn + fPixelSelectionOffset;
		}
	else
		{
		fPixelSelectionPixmap = ZDCPixmap();
		fPixelSelectionRgn = ZDCRgn();
		fPixelSelectionOffset = ZPoint::sZero;
		}

	if (inPaintData)
		{
		changedRgn |= fTextPlane->PasteDataAndSelectIt(inPaintData->fStrings, inPaintData->fFonts, inPaintData->fBounds, inPaintData->fColors);
		}
	else
		{
		vector<string> emptyStrings;
		vector<ZDCFont> emptyFonts;
		vector<ZRect> emptyBounds;
		vector<ZRGBColor> emptyColors;
		changedRgn |= fTextPlane->PasteDataAndSelectIt(emptyStrings, emptyFonts, emptyBounds, emptyColors);
		}

	return changedRgn;
	}

bool NPaintEngine::HasSelection()
	{
	return fTextPlane->HasSelection() || fPixelSelectionRgn;
	}

ZDCRgn NPaintEngine::OffsetSelection(ZPoint inOffset)
	{
	ZDCRgn changedRgn;
	if (inOffset != ZPoint::sZero)
		{
		if (fPixelSelectionRgn)
			{
			changedRgn |= fPixelSelectionRgn + fPixelSelectionOffset;
			fPixelSelectionOffset += inOffset;
			changedRgn |= fPixelSelectionRgn + fPixelSelectionOffset;
			}
		changedRgn |= fTextPlane->OffsetSelection(inOffset);
		}
	return changedRgn;
	}

ZDCRgn NPaintEngine::SetPixelSelectionRgn(const ZDCRgn& inRgn)
	{
	ZDCRgn oldRgn = this->PunchPixelSelectionIntoPixelDC();

	// Clamp the region down to our bounds
	ZDCRgn realRgn = inRgn & ZRect(fSize);

	fPixelSelectionOffset = realRgn.Bounds().TopLeft();
	fPixelSelectionPixmap = fPixelDC.GetPixmap(realRgn.Bounds());
	fPixelSelectionRgn = realRgn - realRgn.Bounds().TopLeft();

	ZDC localDC(fPixelDC);
	// localDC.SetInk(fPaintState->GetInk_Fill());
	localDC.SetInk(ZRGBColor::sWhite);
	localDC.Fill(realRgn);

	return oldRgn | realRgn;
	}

ZDCRgn NPaintEngine::SetPixelSelectionPoints(const vector<ZPoint>& inPixelSelectionPoints)
	{
	ZDCRgn theRgn;
	if (inPixelSelectionPoints.size())
		{
		ZPoint lastPoint = inPixelSelectionPoints[0];
		for (size_t x = 1; x < inPixelSelectionPoints.size(); ++x)
			{
			ZPoint currentPoint = inPixelSelectionPoints[x];
			ZDCRgn tempRgn = ZUtil_Graphics::sCalculateRgnStrokedByLine(lastPoint.h, lastPoint.v, currentPoint.h, currentPoint.v, 1);
			theRgn |= tempRgn;
			lastPoint = currentPoint;
			}
		theRgn |= ZDCRgn::sPoly(&inPixelSelectionPoints[0], inPixelSelectionPoints.size(), false);
		}
	return this->SetPixelSelectionRgn(theRgn);
	}

ZDCRgn NPaintEngine::GetPixelSelectionRgn()
	{
	return fPixelSelectionRgn + fPixelSelectionOffset;
	}

static ZRect sGetPixelSelectionBounds(const vector<ZPoint>& inVector)
	{
	ZRect theBounds = ZRect::sZero;
	if (inVector.size())
		{
		theBounds.left = inVector[0].h;
		theBounds.top = inVector[0].v;
		theBounds.right = theBounds.left;
		theBounds.bottom = theBounds.top;
		for (size_t x = 0; x < inVector.size(); ++x)
			{
			if (theBounds.left > inVector[x].h)
				theBounds.left = inVector[x].h;
			if (theBounds.right < inVector[x].h + 1)
				theBounds.right = inVector[x].h + 1;
			if (theBounds.top > inVector[x].v)
				theBounds.top = inVector[x].v;
			if (theBounds.bottom < inVector[x].v + 1)
				theBounds.bottom = inVector[x].v + 1;
			}
		}
	return theBounds;
	}

ZDCRgn NPaintEngine::SetPixelSelectionFeedback(const vector<ZPoint>& inVector)
	{
	ZRect oldBounds = sGetPixelSelectionBounds(fPixelSelectionFeedback);
	ZRect newBounds = sGetPixelSelectionBounds(inVector);
	fPixelSelectionFeedback = inVector;
	return ZDCRgn(oldBounds) | ZDCRgn(newBounds);
	}

ZDCRgn NPaintEngine::PunchPixelSelectionIntoPixelDC()
	{
	if (fPixelSelectionPixmap && fPixelSelectionRgn)
		{
		ZDCRgn changedRgn = fPixelSelectionRgn + fPixelSelectionOffset;
		ZDC localDC = fPixelDC;
		localDC.SetClip(localDC.GetClip() & changedRgn);
		localDC.Draw(fPixelSelectionOffset, fPixelSelectionPixmap);
		return changedRgn;
		}
	return ZDCRgn();
	}

bool NPaintEngine::GetCursor(CursorName inCursorName, ZCursor& outCursor)
	{ return fPaintState->GetCursor(inCursorName, outCursor); }

void NPaintEngine::PostAction(Action* inAction)
	{
	if (fLastAction)
		{
		if (fLastActionDone)
			{
			try
				{
				fLastAction->Commit(this);
				}
			catch (...)
				{}
			}
		delete fLastAction;
		fLastAction = nil;
		}
	if (inAction)
		{
		try
			{
			inAction->Do(this);
			fLastAction = inAction;
			fLastActionDone = true;
			++fChangeCount;
			}
		catch (...)
			{}
		}
	}

void NPaintEngine::CommitLastAction()
	{
	this->PostAction(nil); 
	}

const ZDC& NPaintEngine::GetPixelDC()
	{
	return fPixelDC;
	}

ZDCRgn NPaintEngine::SwapDCs(ZDC& inOutDC)
	{
	ZDC tempDC(inOutDC);
	inOutDC = fPixelDC;
	fPixelDC = tempDC;
	return inOutDC.GetClip();
	}

ZRef<NPaintEngine::PaintState> NPaintEngine::GetPaintState()
	{ return fPaintState; }

void NPaintEngine::Resize(ZPoint iOffset, ZPoint iSize)
	{
	this->CommitLastAction();

	if (fSize != iSize || iOffset)
		{
		ZDC oldDC(fPixelDC);
		fPixelDC = ZDC_Off(fPixelDC, iSize, ZDCPixmapNS::eFormatEfficient_Color_32);
		fPixelDC.SetInk(ZRGBColor::sWhite);
		fPixelDC.Fill(ZRect(iSize));
		fPixelDC.CopyFrom(iOffset, oldDC, ZRect(fSize));
		fSize = iSize;
		++fChangeCount;
		}

	if (iOffset != ZPoint::sZero)
		{
		fTextPlane->Offset(iOffset);

		++fChangeCount;
		}
	}

bool NPaintEngine::GetTextPlaneActive()
	{
	ToolType theToolType = this->GetTool();
	return theToolType == eToolTypeArrow || theToolType == eToolTypeText;
	}

bool NPaintEngine::GetTextPlaneCanUseTextTool()
	{
	return eToolTypeText == this->GetTool();
	}

//bool NPaintEngine::GetPixelPlaneActive()
//	{
//	}

// ========================================================================================

void NPaintEngine::sExtractMonoPatterns(const ZDCPixmap& inPixmap, vector<ZDCPattern>& outPatterns)
	{
	// Patterns are 8 x 8 pixels. inPixmap is assumed to be made up of a packed grid of 8 x 8
	// blocks. We don't require any particular depth from inPixmap, but any color info is
	// discarded, so it might as well be mono.
	size_t xCount = inPixmap.Width() / 8;
	size_t yCount = inPixmap.Height() / 8;
	for (size_t y = 0; y < yCount; ++y)
		{
		for (size_t x = 0; x < xCount; ++ x)
			{
			ZDCPattern aPattern;
			for (ZCoord v = 0; v < 8; ++v)
				{
				unsigned char currentChar = 0;
				for (ZCoord h = 0; h < 8; ++h)
					{
					if (inPixmap.GetPixel(8 * x + h, 8 * y + v) != ZRGBColor::sWhite)
						currentChar |= (1 << (7-h));
					}
				aPattern.pat[v] = currentChar;
				}
			outPatterns.push_back(aPattern);
			}
		}
	}

void NPaintEngine::sExtractColors(const ZDCPixmap& inPixmap, ZCoord inCountH, ZCoord inCountV, vector<ZRGBColor>& outColors)
	{
	ZCoord hCellWidth = inPixmap.Width() / inCountH;
	ZCoord vCellWidth = inPixmap.Height() / inCountV;
	for (ZCoord y = 0; y < inCountV; ++y)
		{
		for (ZCoord x = 0; x < inCountH; ++x)
			outColors.push_back(inPixmap.GetPixel(x * hCellWidth + hCellWidth / 2, y * vCellWidth + vCellWidth / 2));
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::PaintState

NPaintEngine::PaintState::PaintState()
	{}

NPaintEngine::PaintState::~PaintState()
	{}

bool NPaintEngine::PaintState::GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor)
	{ return false; }

// =================================================================================================
#pragma mark -
#pragma mark * NPaintEngine::PaintState_Indirect
NPaintEngine::PaintState_Indirect::PaintState_Indirect(ZRef<NPaintEngine::PaintState> inRealPaintState)
:	fRealPaintState(nil)
	{
	this->SetRealPaintState(inRealPaintState);
	}

NPaintEngine::PaintState_Indirect::~PaintState_Indirect()
	{
	this->SetRealPaintState(ZRef<NPaintEngine::PaintState>());
	}

NPaintEngine::ToolType NPaintEngine::PaintState_Indirect::GetTool()
	{
	return fRealPaintState->GetTool();
	}

ZDCInk NPaintEngine::PaintState_Indirect::GetInk_Stroke()
	{
	return fRealPaintState->GetInk_Stroke();
	}

ZDCRgn NPaintEngine::PaintState_Indirect::GetRgn_Pen()
	{
	return fRealPaintState->GetRgn_Pen();
	}

ZDCRgn NPaintEngine::PaintState_Indirect::GetRgn_Eraser()
	{
	return fRealPaintState->GetRgn_Eraser();
	}

ZDCInk NPaintEngine::PaintState_Indirect::GetInk_Fill()
	{
	return fRealPaintState->GetInk_Fill();
	}

ZCoord NPaintEngine::PaintState_Indirect::GetWidth_Frame()
	{
	return fRealPaintState->GetWidth_Frame();
	}

bool NPaintEngine::PaintState_Indirect::GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor)
	{
	return fRealPaintState->GetCursor(inCursorName, outCursor);
	}

void NPaintEngine::PaintState_Indirect::SetColor_Stroke(const ZRGBColor& inColor)
	{
	fRealPaintState->SetColor_Stroke(inColor);
	}

void NPaintEngine::PaintState_Indirect::SetTool(NPaintEngine::ToolType inToolType)
	{
	fRealPaintState->SetTool(inToolType);
	}

ZInterest* NPaintEngine::PaintState_Indirect::GetInterest_All()
	{
	return &fInterest_All;
	}

ZInterest* NPaintEngine::PaintState_Indirect::GetInterest_Tool()
	{
	return &fInterest_Tool;
	}

ZInterest* NPaintEngine::PaintState_Indirect::GetInterest_TextColor()
	{
	return &fInterest_TextColor;
	}

void NPaintEngine::PaintState_Indirect::InterestChanged(ZInterest* inInterest)
	{
	if (inInterest == fRealPaintState->GetInterest_All())
		fInterest_All.NotifyClientsImmediate();
	else if (inInterest == fRealPaintState->GetInterest_Tool())
		fInterest_Tool.NotifyClientsImmediate();
	else if (inInterest == fRealPaintState->GetInterest_TextColor())
		fInterest_TextColor.NotifyClientsImmediate();
	}

void NPaintEngine::PaintState_Indirect::SetRealPaintState(ZRef<NPaintEngine::PaintState> inRealPaintState)
	{
	if (fRealPaintState == inRealPaintState)
		return;
	if (fRealPaintState)
		{
		fRealPaintState->GetInterest_All()->UnregisterClient(this);
		fRealPaintState->GetInterest_Tool()->UnregisterClient(this);
		fRealPaintState->GetInterest_TextColor()->UnregisterClient(this);
		}
	fRealPaintState = inRealPaintState;
	if (fRealPaintState)
		{
		fRealPaintState->GetInterest_All()->RegisterClient(this);
		fRealPaintState->GetInterest_Tool()->RegisterClient(this);
		fRealPaintState->GetInterest_TextColor()->RegisterClient(this);
		}
	fInterest_All.NotifyClientsImmediate();
	}

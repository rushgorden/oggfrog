#include "NPainterStd_UI.h"

#include "ZApp.h"
#include "ZMouseTracker.h"
#include "ZUIFactory.h"
#include "ZUIGridPane.h"
#include "ZUtil_UI.h"

#include "ZCompat_cmath.h" // for sqrt

namespace NPainterStd_UI {

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::UIPaintState

UIPaintState::UIPaintState(const ZAsset& inCursorAsset, const vector<ZDCPattern>& inMonoPatterns, const vector<ZRGBColor>& inColorTable,
				const vector<ZDCPixmap>& inPixmapPatterns, const vector<ZDCRgn>& inPenShapes)
:	fCursorAsset(inCursorAsset),
	fVector_Patterns(inMonoPatterns),
	fVector_Colors(inColorTable),
	fVector_Pixmaps(inPixmapPatterns),
	fVector_PenShapes(inPenShapes)
	{
	//fSelectedTool = NPaintEngine::eToolTypeBrush;
	fSelectedTool = NPaintEngine::eToolTypeArrow; // this seems like a "safer" default tool. -ec 99.10.22

	fChipSelections[eChipNameStrokeFore] = this->LookupColor(ZRGBColor::sBlack);
	fChipSelections[eChipNameStrokeBack] = this->LookupColor(ZRGBColor::sWhite);
	fChipSelections[eChipNameStrokePattern] = 0;

	fChipSelections[eChipNameFillFore] = this->LookupColor(ZRGBColor::sBlack);
	fChipSelections[eChipNameFillBack] = this->LookupColor(ZRGBColor::sWhite);
	fChipSelections[eChipNameFillPattern] = 0;

	fChipSelections[eChipNamePenShape] = 0;
	}

UIPaintState::~UIPaintState()
	{
	}

NPaintEngine::ToolType UIPaintState::GetTool()
	{ return fSelectedTool; }

void UIPaintState::SetTool(NPaintEngine::ToolType inToolType)
	{
	if (fSelectedTool != inToolType)
		{
		fSelectedTool = inToolType;
		fInterest_Tool.NotifyClientsImmediate();
		}
	}

ZDCInk UIPaintState::GetInk_Stroke()
	{
	if (fChipSelections[eChipNameStrokePattern] >= fVector_Patterns.size())
		{
		if (fChipSelections[eChipNameStrokePattern] == fVector_Patterns.size() + fVector_Pixmaps.size())
			return ZDCInk();
		ZAssert(fChipSelections[eChipNameStrokePattern] - fVector_Patterns.size() < fVector_Pixmaps.size());
		return fVector_Pixmaps[fChipSelections[eChipNameStrokePattern] - fVector_Patterns.size()];
		}
	ZAssert(fChipSelections[eChipNameStrokeFore]  < fVector_Colors.size());
	ZAssert(fChipSelections[eChipNameStrokeBack] < fVector_Colors.size());
	ZAssert(fChipSelections[eChipNameStrokePattern] < fVector_Patterns.size());
	ZDCPattern thePattern = fVector_Patterns[fChipSelections[eChipNameStrokePattern]];
	ZRGBColor theForeColor = fVector_Colors[fChipSelections[eChipNameStrokeFore]];
	ZRGBColor theBackColor = fVector_Colors[fChipSelections[eChipNameStrokeBack]];

	bool allSet = true;
	bool allClear = true;
	for (int x = 0; x < 8; ++x)
		{
		if (thePattern.pat[x] != 0xFFU)
			allSet = false;
		if (thePattern.pat[x] != 0x0U)
			allClear = false;
		}
	if (allSet)
		return ZDCInk(theForeColor);
	if (allClear)
		return ZDCInk(theBackColor);
	return ZDCInk(theForeColor, theBackColor, thePattern);

//	return ZDCInk(fVector_Colors[fChipSelections[eChipNameStrokeFore]], fVector_Colors[fChipSelections[eChipNameStrokeBack]], fVector_Patterns[fChipSelections[eChipNameStrokePattern]]);
	}

ZRGBColor UIPaintState::GetColor_Text()
	{
// the text color will be the foreground color
	ZAssert(fChipSelections[eChipNameStrokeFore] <= fVector_Colors.size());
	return fVector_Colors[fChipSelections[eChipNameStrokeFore]];
	}

ZDCRgn UIPaintState::GetRgn_Pen()
	{
	if (fChipSelections[eChipNameStrokePattern] == fVector_Patterns.size() + fVector_Pixmaps.size())
		return ZDCRgn();
	ZAssert(fChipSelections[eChipNamePenShape] < fVector_PenShapes.size());
	return fVector_PenShapes[fChipSelections[eChipNamePenShape]];
	}

ZDCRgn UIPaintState::GetRgn_Eraser()
	{
	return ZRect(-8, -8, 8, 8);
//	return this->GetRgn_Pen();
	}

ZDCInk UIPaintState::GetInk_Fill()
	{
	return this->GetInk_Stroke();

	if (fChipSelections[eChipNameFillPattern] >= fVector_Patterns.size())
		{
		if (fChipSelections[eChipNameFillPattern] == fVector_Patterns.size() + fVector_Pixmaps.size())
			return ZDCInk();
		ZAssert(fChipSelections[eChipNameFillPattern] - fVector_Patterns.size() < fVector_Pixmaps.size());
		return fVector_Pixmaps[fChipSelections[eChipNameFillPattern] - fVector_Patterns.size()];
		}
	ZAssert(fChipSelections[eChipNameFillFore]  < fVector_Colors.size());
	ZAssert(fChipSelections[eChipNameFillBack] < fVector_Colors.size());
	ZAssert(fChipSelections[eChipNameFillPattern] < fVector_Patterns.size());
	return ZDCInk(fVector_Colors[fChipSelections[eChipNameFillFore]], fVector_Colors[fChipSelections[eChipNameFillBack]], fVector_Patterns[fChipSelections[eChipNameFillPattern]]);
	}

ZCoord UIPaintState::GetWidth_Frame()
	{
	if (fChipSelections[eChipNameStrokePattern] == fVector_Patterns.size() + fVector_Pixmaps.size())
		return 0;

	ZAssert(fChipSelections[eChipNamePenShape] < fVector_PenShapes.size());
	ZRect currentPenBounds = fVector_PenShapes[fChipSelections[eChipNamePenShape]].Bounds();
	return max(currentPenBounds.Width(), currentPenBounds.Height());
	}

bool UIPaintState::GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor)
	{
	switch (inCursorName)
		{
		case NPaintEngine::eCursorNameArrow:
			break;
		case NPaintEngine::eCursorNameDropper:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Dropper"));
			return true;
		case NPaintEngine::eCursorNameFill:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("PaintBucket"));
			return true;
		case NPaintEngine::eCursorNameEraser:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Eraser"));
			return true;
		case NPaintEngine::eCursorNameSelect:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Select"));
			return true;
		case NPaintEngine::eCursorNameLasso:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Lasso"));
			return true;
		case NPaintEngine::eCursorNamePaintBrush:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("PaintBrush"));
			return true;
		case NPaintEngine::eCursorNameCrossHairs:
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("CrossHairs"));
			return true;
		}
	
	outCursor = ZCursor(ZCursor::eCursorTypeArrow);
	return true;
	}

void UIPaintState::SetColor_Stroke(const ZRGBColor& inColor)
	{
	size_t index = this->LookupColor(inColor);
	this->SetChipSelectedIndex(eChipNameStrokeFore, index);
	}

ZInterest* UIPaintState::GetInterest_All()
	{
	return &fInterest_All;
	}

ZInterest* UIPaintState::GetInterest_Tool()
	{
	return &fInterest_Tool;
	}

ZInterest* UIPaintState::GetInterest_TextColor()
	{
	return GetInterest_Chip(eChipNameStrokeFore);
	}

ZDCInk UIPaintState::GetChipInk(ChipName inChipName)
	{
	ZAssert(inChipName >= eChipNameMIN && inChipName < eChipNameMAX);
	return this->GetChipInk(inChipName, fChipSelections[inChipName]);
	}

ZDCInk UIPaintState::GetChipInk(ChipName inChipName, size_t inIndex)
	{
	ZAssert(inChipName >= eChipNameMIN && inChipName < eChipNameMAX);
	if (inChipName == eChipNameStrokePattern || inChipName == eChipNameFillPattern)
		{
		size_t foreIndex = inChipName == eChipNameStrokePattern ? fChipSelections[eChipNameStrokeFore] : fChipSelections[eChipNameFillFore];
		size_t backIndex = inChipName == eChipNameStrokePattern ? fChipSelections[eChipNameStrokeBack] : fChipSelections[eChipNameFillBack];
		if (inIndex >= fVector_Patterns.size())
			{
			if (inIndex == fVector_Patterns.size() + fVector_Pixmaps.size())
				return ZDCInk();
			ZAssert(inIndex - fVector_Patterns.size() < fVector_Pixmaps.size());
			return fVector_Pixmaps[inIndex - fVector_Patterns.size()];
			}
		ZAssert(foreIndex < fVector_Colors.size());
		ZAssert(backIndex < fVector_Colors.size());
		ZAssert(inIndex < fVector_Patterns.size());
		return ZDCInk(fVector_Colors[foreIndex], fVector_Colors[backIndex], fVector_Patterns[inIndex]);
		}
	ZAssert(inIndex < fVector_Colors.size());
	return fVector_Colors[inIndex];
	}

size_t UIPaintState::GetChipMaxIndex(ChipName inChipName)
	{
	if (inChipName == eChipNamePenShape)
		return fVector_PenShapes.size();

	if (inChipName == eChipNameStrokePattern || inChipName == eChipNameFillPattern)
		return fVector_Patterns.size() + fVector_Pixmaps.size() + 1;

	return fVector_Colors.size();
	}

size_t UIPaintState::GetChipSelectedIndex(ChipName inChipName)
	{
	ZAssert(inChipName >= eChipNameMIN && inChipName < eChipNameMAX);
	return fChipSelections[inChipName];
	}

void UIPaintState::SetChipSelectedIndex(ChipName inChipName, size_t inIndex)
	{
	ZAssert(inChipName >= eChipNameMIN && inChipName < eChipNameMAX);
	if (fChipSelections[inChipName] != inIndex)
		{
		fChipSelections[inChipName] = inIndex;
		fInterests_Chip[inChipName].NotifyClientsImmediate();
		}
	}

ZInterest* UIPaintState::GetInterest_Chip(ChipName inChipName)
	{
	ZAssert(inChipName >= eChipNameMIN && inChipName < eChipNameMAX);
	return &fInterests_Chip[inChipName];
	}

size_t UIPaintState::LookupColor(const ZRGBColor& inColor)
	{
	double targetRed = inColor.red;
	double targetGreen = inColor.green;
	double targetBlue = inColor.blue;

	size_t nearestIndex = 0;
	double nearestDistance = 3.0 * 65536.0 * 65536.0;
	for (size_t x = 0; x < fVector_Colors.size(); ++x)
		{
		double currentRed = fVector_Colors[x].red;
		double currentGreen = fVector_Colors[x].green;
		double currentBlue = fVector_Colors[x].blue;
		double currentDistance = ((currentRed - targetRed) * (currentRed- targetRed)) +
							((currentGreen - targetGreen) * (currentGreen - targetGreen)) +
							((currentBlue- targetBlue) * (currentBlue- targetBlue));
		if (nearestDistance >= currentDistance)
			{
			nearestDistance = currentDistance;
			nearestIndex = x;
			}
		}
	return nearestIndex;
	}

void UIPaintState::DrawPaletteCell(const ZDC& inDC, const ZRect& inBounds, ChipName inChipName, size_t inIndex)
	{
	ZDC localDC(inDC);
	if (inChipName == eChipNamePenShape)
		{
		localDC.SetClip(inBounds);
		localDC.SetInk(ZRGBColor::sWhite);
		localDC.Fill(inBounds);
		ZDCRgn theRgn(fVector_PenShapes[inIndex]);
		ZRect penBounds = theRgn.Bounds();
		ZRect centredPenBounds = penBounds.Centered(inBounds);
		localDC.SetInk(ZRGBColor::sBlack);
		localDC.Fill(theRgn + (centredPenBounds.TopLeft() - penBounds.TopLeft()));
		}
	else
		{
		ZDCInk theInk = this->GetChipInk(inChipName, inIndex);
		if (!theInk)
			{
			localDC.SetInk(ZRGBColor::sWhite);
			localDC.Fill(inBounds);
			string noneString = "None";
			localDC.SetFont(ZDCFont::sApp9);
			ZCoord textWidth = localDC.GetTextWidth(noneString);
			ZCoord ascent, descent, leading;
			localDC.GetFontInfo(ascent, descent, leading);
			localDC.Draw(ZPoint(inBounds.left + (inBounds.Width() - textWidth) / 2, inBounds.top + (inBounds.Height() - ascent - descent) / 2 + ascent), noneString);
			}
		else
			{
			localDC.SetInk(theInk);
			localDC.Fill(inBounds);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::InkPaletteWindow Interface

#if NPainterConfig_UseWindows

class InkPaletteWindow : public ZWindow,
		public ZPaneLocator,
		public ZUIGridPane::CellRenderer,
		public ZUIGridPane::Geometry_Fixed,
		public ZUIGridPane::Selection_Single,
		public ZInterestClient
	{
public:
	static ZOSWindow* sCreateOSWindow(bool inIsMenu);
	InkPaletteWindow(ZPoint inGlobalLocation, const ZRef<UIPaintState>& inUIPaintState, ChipName inChipName);
	virtual ~InkPaletteWindow();

// From ZPaneLocator
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);

// From ZUIGridPane::CellRenderer
	virtual void DrawCell(const ZDC& inDC, ZPoint_T<int32> inCell, ZRect inCellBounds, bool inSelected, bool inIsAnchor);

// From ZUIGridPane::Geometry_Fixed
	virtual int32 GetCellCount(bool inHorizontal);
	virtual ZCoord GetCellSize(bool inHorizontal);

// From ZUIGridPane::Selection via ZUIGridPane::Selection_Single
	virtual bool GetSelected(ZPoint_T<int32> inCell);
	virtual bool Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// From ZUIGridPane::Selection_Single
	virtual ZPoint_T<int32> GetSelectedCell();
	virtual void SetSelectedCell(ZUIGridPane* inGridPane, ZPoint_T<int32> inCell, bool inNotify);
	virtual void SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify);

// From ZInterestClient
	virtual void InterestChanged(ZInterest* inInterest);

protected:
	ZRef<UIPaintState> fUIPaintState;
	ChipName fChipName;

	ZWindowPane* fWindowPane;
	ZUIGridPane* fGridPane;

	int32 fColumnCount;
	ZPoint_T<int32> fSelectedCell;
	bool fSelectedCellValid;

	bool fIsMenu;
	class SelectionTracker;
	friend class SelectionTracker;
	};

#endif // NPainterConfig_UseWindows

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::InkPaletteWindow::SelectionTracker

#if NPainterConfig_UseWindows

class InkPaletteWindow::SelectionTracker : public ZMouseTracker
	{
public:
	SelectionTracker(InkPaletteWindow* inWindow, ZPoint inHitPoint);
	virtual ~SelectionTracker();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

protected:
	InkPaletteWindow* fWindow;
	ZPoint_T<int32> fInitialSelectedCell;
	};

InkPaletteWindow::SelectionTracker::SelectionTracker(InkPaletteWindow* inWindow, ZPoint inHitPoint)
:	ZMouseTracker(inWindow->fGridPane, inHitPoint), fWindow(inWindow)
	{
	fInitialSelectedCell = fWindow->GetSelectedCell();
	}

InkPaletteWindow::SelectionTracker::~SelectionTracker()
	{
	ZAssert(fWindow != nil);
	fWindow = nil;
	}

void InkPaletteWindow::SelectionTracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZPoint_T<int32> maxCell = fWindow->fGridPane->GetMaxCell();
	ZPoint_T<int32> hitCell = fWindow->fGridPane->FindCell(inNext, false);
	if (hitCell.h < 0 || hitCell.v < 0 || hitCell.h >= maxCell.h || hitCell.v >= maxCell.v)
		{
		hitCell = fInitialSelectedCell;
		}

	ZPoint_T<int32> lastHitCell = fWindow->GetSelectedCell();
	if (lastHitCell != hitCell)
		{
		fWindow->SetSelectedCell(fWindow->fGridPane, hitCell, false);
//		fWindow->UpdateWindowNow();
		}
	if (fWindow->fIsMenu && inPhase == ePhaseEnd)
		fWindow->CloseAndDispose();
	}

#endif // NPainterConfig_UseWindows

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::InkPaletteWindow Implementation

#if NPainterConfig_UseWindows

ZOSWindow* InkPaletteWindow::sCreateOSWindow(bool inIsMenu)
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0, 0, 300, 300);
	theAttributes.fLook = inIsMenu ? ZOSWindow::lookThinBorder : ZOSWindow::lookPalette;
	theAttributes.fLayer = inIsMenu ? ZOSWindow::layerMenu : ZOSWindow::layerFloater;
	theAttributes.fResizable = false;
	theAttributes.fHasSizeBox = false;
	theAttributes.fHasCloseBox = false;
	theAttributes.fHasZoomBox = false;
	theAttributes.fHasMenuBar = false;
	theAttributes.fHasTitleIcon = false;

	return ZApp::sGet()->CreateOSWindow(theAttributes);
	}

InkPaletteWindow::InkPaletteWindow(ZPoint inGlobalLocation, const ZRef<UIPaintState>& inUIPaintState, ChipName inChipName)
:	ZWindow(ZApp::sGet(), sCreateOSWindow(true)), ZPaneLocator(nil),
	fWindowPane(nil),
	fUIPaintState(inUIPaintState),
	fChipName(inChipName),
	fIsMenu(true)
	{
	this->SetBackInk(ZUIAttributeFactory::sGet()->GetInk_WindowBackground_Dialog());
	fWindowPane = new ZWindowPane(this, nil);

	fSelectedCellValid = false;

	fUIPaintState->GetInterest_Chip(fChipName)->RegisterClient(this);

	if (fChipName == eChipNameStrokePattern)
		{
		fUIPaintState->GetInterest_Chip(eChipNameStrokeFore)->RegisterClient(this);
		fUIPaintState->GetInterest_Chip(eChipNameStrokeBack)->RegisterClient(this);
		}

	if (fChipName == eChipNameFillPattern)
		{
		fUIPaintState->GetInterest_Chip(eChipNameFillFore)->RegisterClient(this);
		fUIPaintState->GetInterest_Chip(eChipNameFillBack)->RegisterClient(this);
		}

	size_t maxIndex = fUIPaintState->GetChipMaxIndex(fChipName);
	if (fChipName == eChipNamePenShape)
		fColumnCount = maxIndex / 2;
	else
		fColumnCount = long(sqrt(double(maxIndex)));

	fGridPane = new ZUIGridPane(fWindowPane, this, this, this, this, this);

	ZRect theBounds;
	ZPoint_T<int32> maxCell = fGridPane->GetMaxCell();
	fGridPane->GetCellRect(ZPoint_T<int32>(maxCell.h - 1, maxCell.v -1), theBounds);
	this->SetSize(theBounds.BottomRight());

	this->SetLocation(inGlobalLocation);

	this->BringFront();

	if (fIsMenu)
		{
		ZMouseTracker* theTracker = new SelectionTracker(this, ZPoint(-16000, -16000));
		theTracker->Install();
		}

	this->GetWindowLock().Release();
	}

InkPaletteWindow::~InkPaletteWindow()
	{
	delete fGridPane;
	}

bool InkPaletteWindow::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool InkPaletteWindow::Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZMouseTracker* theTracker = new InkPaletteWindow::SelectionTracker(this, inHitPoint);
	theTracker->Install();
	return true;
	}

bool InkPaletteWindow::GetSelected(ZPoint_T<int32> inCell)
	{
	size_t selectedChipIndex = fUIPaintState->GetChipSelectedIndex(fChipName);
	return selectedChipIndex == (inCell.h + inCell.v * fColumnCount);
	}

ZPoint_T<int32> InkPaletteWindow::GetSelectedCell()
	{
	if (!fSelectedCellValid)
		{
		size_t selectedChipIndex = fUIPaintState->GetChipSelectedIndex(fChipName);
		fSelectedCell.h = selectedChipIndex % fColumnCount;
		fSelectedCell.v = selectedChipIndex / fColumnCount;
		fSelectedCellValid = true;
		}
	return fSelectedCell;
	}

void InkPaletteWindow::SetSelectedCell(ZUIGridPane* inGridPane, ZPoint_T<int32> inCell, bool inNotify)
	{
	if (inCell.h < 0 || inCell.h >= fColumnCount)
		return;
	size_t maxIndex = fUIPaintState->GetChipMaxIndex(fChipName);
	int32 maxVCoord = (maxIndex + fColumnCount - 1) / fColumnCount;
	if (inCell.v < 0 || inCell.v >= maxVCoord)
		return;

	size_t selectedChipIndex = (inCell.h + inCell.v * fColumnCount);
	if (selectedChipIndex >= maxIndex)
		return;
	fUIPaintState->SetChipSelectedIndex(fChipName, selectedChipIndex);
	}

void InkPaletteWindow::SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify)
	{
	}

void InkPaletteWindow::DrawCell(const ZDC& inDC, ZPoint_T<int32> inCell, ZRect inCellBounds, bool inSelected, bool inIsAnchor)
	{
	ZDC localDC = inDC;
	size_t maxIndex = fUIPaintState->GetChipMaxIndex(fChipName);
	size_t theCellIndex = inCell.v * fColumnCount + inCell.h;
	if (theCellIndex < maxIndex)
		{
		if (inSelected)
			{
			localDC.SetInk(ZRGBColor::sBlack);
			localDC.SetPenWidth(2);
			localDC.Frame(inCellBounds);
			}
		else
			{
			localDC.SetInk(ZRGBColor::sBlack);
			localDC.SetPenWidth(1);
			localDC.Frame(inCellBounds.Inset(1,1));
			localDC.SetInk(ZRGBColor::sWhite);
			localDC.Frame(inCellBounds);
			}

		fUIPaintState->DrawPaletteCell(localDC, inCellBounds.Inset(2, 2), fChipName, theCellIndex);
		}
	else
		{
		localDC.SetInk(ZRGBColor::sWhite);
		localDC.Frame(inCellBounds);
		localDC.SetInk(ZDCInk::sGray);
		localDC.Fill(inCellBounds.Inset(1, 1));
		}
	}

int32 InkPaletteWindow::GetCellCount(bool inHorizontal)
	{
	if (inHorizontal)
		return fColumnCount;
	size_t maxIndex = fUIPaintState->GetChipMaxIndex(fChipName);
	return (maxIndex + fColumnCount - 1) / fColumnCount;
	}

ZCoord InkPaletteWindow::GetCellSize(bool inHorizontal)
	{
	if (fChipName == eChipNamePenShape || fChipName == eChipNameStrokePattern || fChipName == eChipNameFillPattern)
		return 32;

	return 16;
	}

// From ZInterestClient
void InkPaletteWindow::InterestChanged(ZInterest* inInterest)
	{
	ZLocker locker(this->GetWindowLock());
	if (fChipName == eChipNameStrokePattern)
		{
		if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameStrokeFore))
			fGridPane->Refresh();
		if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameStrokeBack))
			fGridPane->Refresh();
		}
	if (fChipName == eChipNameFillPattern)
		{
		if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameFillFore))
			fGridPane->Refresh();
		if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameFillBack))
			fGridPane->Refresh();
		}

	if (inInterest == fUIPaintState->GetInterest_Chip(fChipName))
		{
		fGridPane->InvalidateCell(this->GetSelectedCell());
		fSelectedCellValid = false;
		fGridPane->InvalidateCell(this->GetSelectedCell());
		}
//	this->UpdateWindowNow();
	}

#endif // NPainterConfig_UseWindows

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::CaptionChip

class CaptionChip : public ZUICaption
	{
public:
	CaptionChip(const ZDCInk& inInk, ZPoint inSize);
	virtual ~CaptionChip();

// From ZUICaption
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

protected:
	ZDCInk fInk;
	ZPoint fSize;
	};

CaptionChip::CaptionChip(const ZDCInk& inInk, ZPoint inSize)
:	fInk(inInk), fSize(inSize)
	{}

CaptionChip::~CaptionChip()
	{
	}

void CaptionChip::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZDC localDC(inDC);
	localDC.SetInk(fInk);
	localDC.Fill(ZRect(fSize) + inLocation);
	}

ZDCRgn CaptionChip::GetRgn()
	{ return ZRect(fSize); }

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::CaptionPen

class CaptionPen : public ZUICaption
	{
public:
	CaptionPen(const ZDCRgn& inPenRgn, const ZDCInk& inPenInk, ZPoint inCaptionSize);
	virtual ~CaptionPen();

// From ZUICaption
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

protected:
	ZDCRgn fPenRgn;
	ZDCInk fPenInk;
	ZPoint fCaptionSize;
	};

CaptionPen::CaptionPen(const ZDCRgn& inPenRgn, const ZDCInk& inPenInk, ZPoint inCaptionSize)
:	fPenRgn(inPenRgn), fPenInk(inPenInk), fCaptionSize(inCaptionSize)
	{}

CaptionPen::~CaptionPen()
	{}

void CaptionPen::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZDC localDC(inDC);
	ZRect captionRect(fCaptionSize);
	captionRect += inLocation;
	ZRect penBounds = fPenRgn.Bounds();
	ZRect centredPenBounds = penBounds.Centered(captionRect);
	localDC.SetInk(fPenInk);
	localDC.Fill(fPenRgn + centredPenBounds.TopLeft() - penBounds.TopLeft());
	}

ZDCRgn CaptionPen::GetRgn()
	{ return ZRect(fCaptionSize); }

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::ToolPane

static ZUIButton* sCreateButton(ZSuperPane* inSuperPane, ZPaneLocator* inLocator,
				ZUIButton::Responder* inResponder, const ZAsset& inAsset, const char* inName)
	{
	ZRef<ZUICaption> thePixCaption = new ZUICaption_Pix(ZUtil_UI::sGetPixmapCombo(inAsset.GetChild(inName)));
	return ZUIFactory::sGet()->Make_ButtonBevel(inSuperPane, inLocator, inResponder, thePixCaption);
	}

static const ZCoord kToolCaptionWidth = 24;
static const ZCoord kToolCaptionHeight = 18;
static const ZCoord kToolButtonWidth = kToolCaptionWidth+4;
static const ZCoord kToolButtonHeight = kToolCaptionHeight+4;
static const ZCoord kToolButtonSpace = 2;

static const ZCoord kToolWindowWidth = kToolButtonSpace + 3 * (kToolButtonWidth + kToolButtonSpace);
static const ZCoord kToolWindowHeight = kToolButtonSpace + 7 * (kToolButtonHeight + kToolButtonSpace);

ToolPane::ToolPane(ZSuperPane* iSuperPane, ZPaneLocator* iPaneLocator, const ZRef<UIPaintState>& inUIPaintState, const ZAsset& inButtonAsset)
:	ZSuperPane(iSuperPane, iPaneLocator),
	ZPaneLocator(nil),
	fUIPaintState(inUIPaintState)
	{
	fHighlightedToolButton = nil;

	ZRef<ZUIFactory> theUIFactory = ZUIFactory::sGet();

// Build all our buttons
#if 1
	fCaption_StrokeFore = new ZUICaption_Dynamic(this);
	fButton_StrokeFore = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_StrokeFore);
	fCaption_StrokePattern = new ZUICaption_Dynamic(this);
	fButton_StrokePattern = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_StrokePattern);
	fCaption_StrokeBack = new ZUICaption_Dynamic(this);
	fButton_StrokeBack = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_StrokeBack);
#endif

	fButton_Arrow = sCreateButton(this, this, this, inButtonAsset, "Arrow");
	fButton_Text = sCreateButton(this, this, this, inButtonAsset, "Text");
//	fButton_Hiliter = sCreateButton(this, this, this, inButtonAsset, "Hiliter");
	fButton_Dropper = sCreateButton(this, this, this, inButtonAsset, "Dropper");

	fButton_Fill = sCreateButton(this, this, this, inButtonAsset, "PaintBucket");
	fButton_Brush = sCreateButton(this, this, this, inButtonAsset, "PaintBrush");
	fButton_Rect = sCreateButton(this, this, this, inButtonAsset, "Rect");
	fButton_Oval = sCreateButton(this, this, this, inButtonAsset, "Oval");
	fButton_Polygon = sCreateButton(this, this, this, inButtonAsset, "Polygon");
	fButton_RectFrame = sCreateButton(this, this, this, inButtonAsset, "FrameRect");
	fButton_OvalFrame = sCreateButton(this, this, this, inButtonAsset, "FrameOval");
	fButton_PolyFrame = sCreateButton(this, this, this, inButtonAsset, "FramePoly");
	fButton_Line = sCreateButton(this, this, this, inButtonAsset, "Line");

	fButton_Eraser = sCreateButton(this, this, this, inButtonAsset, "Eraser");
	fButton_Select = sCreateButton(this, this, this, inButtonAsset, "Select");
	fButton_Lasso = sCreateButton(this, this, this, inButtonAsset, "Lasso");

	if (false)
		{
		fCaption_FillFore = new ZUICaption_Dynamic(this);
		fButton_FillFore = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_FillFore);
		fCaption_FillPattern = new ZUICaption_Dynamic(this);
		fButton_FillPattern = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_FillPattern);
		fCaption_FillBack = new ZUICaption_Dynamic(this);
		fButton_FillBack = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_FillBack);
		}
	else
		{
		fButton_FillFore = nil;
		fButton_FillPattern = nil;
		fButton_FillBack = nil;
		}

	fCaption_PenShape = new ZUICaption_Dynamic(this);
	fButton_PenShape = theUIFactory->Make_ButtonBevel(this, this, this, fCaption_PenShape);

	fUIPaintState->GetInterest_Chip(eChipNameStrokeFore)->RegisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameStrokeBack)->RegisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameStrokePattern)->RegisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillFore)->RegisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillBack)->RegisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillPattern)->RegisterClient(this);

	fUIPaintState->GetInterest_Chip(eChipNamePenShape)->RegisterClient(this);

	fUIPaintState->GetInterest_Tool()->RegisterClient(this);
	}

ToolPane::~ToolPane()
	{
	fUIPaintState->GetInterest_Chip(eChipNameStrokeFore)->UnregisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameStrokeBack)->UnregisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameStrokePattern)->UnregisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillFore)->UnregisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillBack)->UnregisterClient(this);
	fUIPaintState->GetInterest_Chip(eChipNameFillPattern)->UnregisterClient(this);

	fUIPaintState->GetInterest_Chip(eChipNamePenShape)->UnregisterClient(this);

	fUIPaintState->GetInterest_Tool()->UnregisterClient(this);
	}

ZPoint ToolPane::GetSize()
	{
	return ZPoint(kToolWindowWidth, kToolWindowHeight);
	}

bool ToolPane::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	if (inPane == fButton_Arrow)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 0 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Text)
		{
		outLocation.h = kToolButtonSpace + 1 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 0 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Fill)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 0 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}
	
	if (inPane == fButton_Brush)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 1 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}
	if (inPane == fButton_PenShape)
		{
		outLocation.h = kToolButtonSpace + 1 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 1 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}
	if (inPane == fButton_Line)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 1 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}


	if (inPane == fButton_RectFrame)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 2 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_OvalFrame)
		{
		outLocation.h = kToolButtonSpace + 1 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 2 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_PolyFrame)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 2 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}


	if (inPane == fButton_Rect)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 3 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Oval)
		{
		outLocation.h = kToolButtonSpace + 1 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 3 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Polygon)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 3 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}


	if (inPane == fButton_Eraser)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 4 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}
	
	if (inPane == fButton_Select)
		{
		outLocation.h = kToolButtonSpace + 1 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 4 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Lasso)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace);
		outLocation.v = kToolButtonSpace + 4 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_Dropper)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace) + 15;
		outLocation.v = kToolButtonSpace + 5 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_StrokeFore)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace) - 15;
		outLocation.v = kToolButtonSpace + 5 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}


	if (inPane == fButton_StrokePattern)
		{
		outLocation.h = kToolButtonSpace + 0 * (kToolButtonWidth + kToolButtonSpace) + 15;
		outLocation.v = kToolButtonSpace + 6 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	if (inPane == fButton_StrokeBack)
		{
		outLocation.h = kToolButtonSpace + 2 * (kToolButtonWidth + kToolButtonSpace) - 15;
		outLocation.v = kToolButtonSpace + 6 * (kToolButtonHeight + kToolButtonSpace);
		return true;
		}

	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool ToolPane::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (inPane == fButton_Arrow || inPane == fButton_Text || inPane == fButton_Fill ||
		inPane == fButton_Brush || inPane == fButton_PenShape || inPane == fButton_Line ||
		inPane == fButton_RectFrame || inPane == fButton_OvalFrame || inPane == fButton_PolyFrame ||
		inPane == fButton_Rect || inPane == fButton_Oval || inPane == fButton_Polygon
		|| inPane == fButton_Eraser || inPane == fButton_Select || inPane == fButton_Lasso
		|| inPane == fButton_Dropper || inPane == fButton_StrokeFore || inPane == fButton_StrokePattern || inPane == fButton_StrokeBack)
		{
		outSize.h = kToolButtonWidth;
		outSize.v = kToolButtonHeight;
		return true;
		}
	return ZPaneLocator::GetPaneSize(inPane, outSize);
	}

bool ToolPane::GetPaneEnabled(ZSubPane* inPane, bool& outEnabled)
	{
	return ZPaneLocator::GetPaneEnabled(inPane, outEnabled);
	}

bool ToolPane::GetPaneHilite(ZSubPane* inPane, EZTriState& outHilited)
	{
//	if (inPane == fButton_Hiliter)
//		{
//		}

	if (inPane == fButton_Text ||
		inPane == fButton_Arrow ||
		inPane == fButton_Dropper ||
		inPane == fButton_Fill ||
		inPane == fButton_Brush ||
		inPane == fButton_Rect ||
		inPane == fButton_Oval ||
		inPane == fButton_Polygon ||
		inPane == fButton_RectFrame ||
		inPane == fButton_OvalFrame ||
		inPane == fButton_PolyFrame ||
		inPane == fButton_Line ||
		inPane == fButton_Eraser ||
		inPane == fButton_Select ||
		inPane == fButton_Lasso)
		{
		outHilited = (inPane == this->GetHighlightedToolButton()) ? eZTriState_On : eZTriState_Off;
		return true;
		}

	return ZPaneLocator::GetPaneHilite(inPane, outHilited);
	}

bool ToolPane::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inAttribute == "ZUIButton::HasPopupArrow")
		{
		if (inPane == fButton_FillFore || inPane == fButton_FillPattern || inPane == fButton_FillBack ||
			inPane == fButton_StrokeFore || inPane == fButton_StrokePattern || inPane == fButton_StrokeBack ||
			inPane == fButton_PenShape)
			{
			(*(bool*)outResult) = true;
			return true;
			}
		}

	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

bool ToolPane::HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent)
	{
	if (inButtonEvent == ZUIButton::ButtonDown)
		{
		ZPoint buttonGlobalLocation = inButton->ToGlobal(inButton->CalcBounds().TopRight());
		#if NPainterConfig_UseWindows
			if (inButton == fButton_FillFore)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameFillFore);
				return true;
				}
			if (inButton == fButton_FillPattern)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameFillPattern);
				return true;
				}
			if (inButton == fButton_FillBack)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameFillBack);
				return true;
				}
			if (inButton == fButton_StrokeFore)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameStrokeFore);
				return true;
				}
			if (inButton == fButton_StrokePattern)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameStrokePattern);
				return true;
				}
			if (inButton == fButton_StrokeBack)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNameStrokeBack);
				return true;
				}
			if (inButton == fButton_PenShape)
				{
				new InkPaletteWindow(buttonGlobalLocation, fUIPaintState, eChipNamePenShape);
				return true;
				}
		#endif // NPainterConfig_UseWindows
		}

	if (inButtonEvent == ZUIButton::ButtonDoubleClick)
		{
		if (inButton == fButton_Eraser)
			{
			return true;
			}
		}

	if (inButtonEvent == ZUIButton::ButtonAboutToBeReleasedWhileDown)
		{
		if (inButton == fButton_Arrow)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeArrow);
			}

		if (inButton == fButton_Text)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeText);
			}
	
		if (inButton == fButton_Dropper)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeDropper);
			}

		if (inButton == fButton_Fill)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeFill);
			}

		if (inButton == fButton_Brush)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeBrush);
			}

		if (inButton == fButton_Rect)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeRect);
			}

		if (inButton == fButton_Oval)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeOval);
			}

		if (inButton == fButton_Polygon)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypePolygon);
			}
		
		if (inButton == fButton_RectFrame)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeRectFrame);
			}

		if (inButton == fButton_OvalFrame)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeOvalFrame);
			}

		if (inButton == fButton_PolyFrame)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypePolygonFrame);
			}

		/*if (inButton == fButton_Freeform)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeFreeform);
			} */

		if (inButton == fButton_Line)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeLine);
			}

		if (inButton == fButton_Eraser)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeEraser);
			}

		if (inButton == fButton_Select)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeSelect);
			}

		if (inButton == fButton_Lasso)
			{
			fUIPaintState->SetTool(NPaintEngine::eToolTypeLasso);
			}
		
//		this->Refresh();
		}

	return ZUIButton::Responder::HandleUIButtonEvent(inButton, inButtonEvent);
	}

ZRef<ZUICaption> ToolPane::ProvideUICaption(ZUICaption_Dynamic* inCaption)
	{
	bool isOval = false;
	ZRef<ZUICaption> theCaption;
	if (fCaption_FillFore == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameFillFore), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}
	else if (fCaption_FillPattern == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameFillPattern), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}
	else if (fCaption_FillBack == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameFillBack), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}

	if (fCaption_StrokeFore == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameStrokeFore), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}
	else if (fCaption_StrokePattern == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameStrokePattern), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}
	else if (fCaption_StrokeBack == inCaption)
		{
		theCaption = new CaptionChip(fUIPaintState->GetChipInk(eChipNameStrokeBack), ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}

	else if (fCaption_PenShape == inCaption)
		{
		theCaption = new CaptionPen(fUIPaintState->GetRgn_Pen(), ZRGBColor::sBlack, ZPoint(kToolCaptionWidth, kToolCaptionHeight));
		}
	return theCaption;
	}

void ToolPane::InterestChanged(ZInterest* inInterest)
	{
	ZLocker locker(this->GetLock());
	if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameStrokeFore))
		{
		fButton_StrokeFore->Refresh();
		fButton_StrokePattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameStrokeBack))
		{
		fButton_StrokeBack->Refresh();
		fButton_StrokePattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameStrokePattern))
		{
		fButton_StrokePattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameFillFore))
		{
		fButton_FillFore->Refresh();
		fButton_FillPattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameFillBack))
		{
		fButton_FillBack->Refresh();
		fButton_FillPattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNameFillPattern))
		{
		fButton_PenShape->Refresh();
		fButton_FillPattern->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Chip(eChipNamePenShape))
		{
		fButton_PenShape->Refresh();
		}
	else if (inInterest == fUIPaintState->GetInterest_Tool())
		{
		this->GetHighlightedToolButton()->Refresh();
		fHighlightedToolButton = nil;
		this->GetHighlightedToolButton()->Refresh();
		}

//	this->UpdateWindowNow();
	}

ZUIButton* ToolPane::GetHighlightedToolButton()
	{
	if (fHighlightedToolButton == nil)
		{
		switch (fUIPaintState->GetTool())
			{
			case NPaintEngine::eToolTypeArrow: fHighlightedToolButton = fButton_Arrow; break;
			case NPaintEngine::eToolTypeText: fHighlightedToolButton = fButton_Text; break;

			case NPaintEngine::eToolTypeDropper: fHighlightedToolButton = fButton_Dropper; break;
			case NPaintEngine::eToolTypeFill: fHighlightedToolButton = fButton_Fill; break;
			case NPaintEngine::eToolTypeBrush: fHighlightedToolButton = fButton_Brush; break;

			case NPaintEngine::eToolTypeRect: fHighlightedToolButton = fButton_Rect; break;
			case NPaintEngine::eToolTypeOval: fHighlightedToolButton = fButton_Oval; break;
			case NPaintEngine::eToolTypePolygon: fHighlightedToolButton = fButton_Polygon; break;

			case NPaintEngine::eToolTypeRectFrame: fHighlightedToolButton = fButton_RectFrame; break;
			case NPaintEngine::eToolTypeOvalFrame: fHighlightedToolButton = fButton_OvalFrame; break;
			case NPaintEngine::eToolTypePolygonFrame: fHighlightedToolButton = fButton_PolyFrame; break;

			case NPaintEngine::eToolTypeLine: fHighlightedToolButton = fButton_Line; break;
			case NPaintEngine::eToolTypeEraser: fHighlightedToolButton = fButton_Eraser; break;

			case NPaintEngine::eToolTypeSelect: fHighlightedToolButton = fButton_Select; break;
			case NPaintEngine::eToolTypeLasso: fHighlightedToolButton = fButton_Lasso; break;
			}
		}
	return fHighlightedToolButton;
	}

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::ToolWindow

#if NPainterConfig_UseWindows

ZOSWindow* ToolWindow::sCreateOSWindow(ZApp* inApp)
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0, 0, kToolWindowWidth, kToolWindowHeight);
	theAttributes.fLook = ZOSWindow::lookPalette;
	theAttributes.fLayer = ZOSWindow::layerFloater;
	theAttributes.fResizable = false;
	theAttributes.fHasSizeBox = false;
	theAttributes.fHasCloseBox = false;
	theAttributes.fHasZoomBox = false;
	theAttributes.fHasMenuBar = false;
	theAttributes.fHasTitleIcon = false;

	return inApp->CreateOSWindow(theAttributes);
	}

ToolWindow::ToolWindow(ZApp* inApp, ZPoint inLocation, const ZRef<UIPaintState>& inUIPaintState, const ZAsset& inButtonAsset)
:	ZWindow(inApp, sCreateOSWindow(inApp))
	{
// Align the top left corner of our OS Window's structure region with the passed in location
	fOSWindow->SetLocation(inLocation + fOSWindow->GetContentTopLeftInset());

	fWindowPane = new ZWindowPane(this, nil);
	this->SetBackInk(ZUIAttributeFactory::sGet()->GetInk_WindowBackground_Dialog());

	fToolPane = new ToolPane(fWindowPane, nil, inUIPaintState, inButtonAsset);

	this->SetSize(fToolPane->GetSize());
	}

ToolWindow::~ToolWindow()
	{
	}

bool ToolWindow::WindowQuitRequested()
	{
	return true;
	}

void ToolWindow::WindowCloseByUser()
	{
	this->Show(false);
	}

#endif // NPainterConfig_UseWindows

} // namespace NPainterStd_UI
// =================================================================================================

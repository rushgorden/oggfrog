#ifndef __NPainterStd_UI__
#define __NPainterStd_UI__ 1
#include "zconfig.h"

#ifndef NPainterConfig_UseWindows
#	define NPainterConfig_UseWindows 1
#endif

#include "NPainter.h"

#include "ZAsset.h"
#include "ZUI.h"

#if NPainterConfig_UseWindows
#	include "ZWindow.h"
class ZApp;
#endif

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI

namespace NPainterStd_UI {

enum ChipName { eChipNameMIN = 0,
			eChipNameStrokeFore = 0, eChipNameStrokeBack, eChipNameStrokePattern,
			eChipNameFillFore, eChipNameFillBack, eChipNameFillPattern,
			eChipNamePenShape,
			eChipNameMAX };

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::UIPaintState

class UIPaintState : public NPaintEngine::PaintState
	{
public:
	UIPaintState(const ZAsset& inCursorAsset, const vector<ZDCPattern>& inMonoPatterns, const vector<ZRGBColor>& inColorTable,
				const vector<ZDCPixmap>& inPixmapPatterns, const vector<ZDCRgn>& inPenShapes);
	virtual ~UIPaintState();

// From NPaintEngine::PaintState
	virtual NPaintEngine::ToolType GetTool();

	virtual ZDCInk GetInk_Stroke();
	virtual ZDCRgn GetRgn_Pen();
	virtual ZDCRgn GetRgn_Eraser();
	virtual ZDCInk GetInk_Fill();
	virtual ZCoord GetWidth_Frame();
	virtual ZRGBColor GetColor_Text();

	virtual bool GetCursor(NPaintEngine::CursorName inCursorName, ZCursor& outCursor);

	virtual void SetColor_Stroke(const ZRGBColor& inColor);

	virtual void SetTool(NPaintEngine::ToolType inToolType);

	virtual ZInterest* GetInterest_All();
	virtual ZInterest* GetInterest_Tool();
	virtual ZInterest* GetInterest_TextColor();

// Our Protocol
	ZDCInk GetChipInk(ChipName inChipName);
	ZDCInk GetChipInk(ChipName inChipName, size_t inIndex);

	size_t GetChipMaxIndex(ChipName inChipName);
	size_t GetChipSelectedIndex(ChipName inChipName);
	void SetChipSelectedIndex(ChipName inChipName, size_t inIndex);

	ZInterest* GetInterest_Chip(ChipName inChipName);

	size_t LookupColor(const ZRGBColor& inColor);

	void DrawPaletteCell(const ZDC& inDC, const ZRect& inBounds, ChipName inChipName, size_t inIndex);

protected:
	ZInterest fInterests_Chip[eChipNameMAX];
	ZInterest fInterest_Tool;

	ZInterest fInterest_All;

	size_t fChipSelections[eChipNameMAX];
	
	ZAsset fCursorAsset;

	vector<ZRGBColor> fVector_Colors;
	vector<ZDCPattern> fVector_Patterns;
	vector<ZDCPixmap> fVector_Pixmaps;
	vector<ZDCRgn> fVector_PenShapes;

	NPaintEngine::ToolType fSelectedTool;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::ToolPane

class ToolPane : public ZSuperPane,
	public ZPaneLocator,
	public ZUIButton::Responder,
	public ZUICaption_Dynamic::Provider,
	public ZInterestClient
	{
public:
	ToolPane(ZSuperPane* iSuperPane, ZPaneLocator* iPaneLocator, const ZRef<UIPaintState>& inUIPaintState, const ZAsset& inButtonAsset);
	virtual ~ToolPane();

// From ZSubPane
	virtual ZPoint GetSize();

// From ZPaneLocator
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
	virtual bool GetPaneEnabled(ZSubPane* inPane, bool& outEnabled);
	virtual bool GetPaneHilite(ZSubPane* inPane, EZTriState& outHilited);
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

// From ZUIButton::Responder
	virtual bool HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent);

// From ZUICaption_Dynamic::Provider
	virtual ZRef<ZUICaption> ProvideUICaption(ZUICaption_Dynamic* inCaption);

// From ZInterestClient
	virtual void InterestChanged(ZInterest* inInterest);

protected:
	ZRef<UIPaintState> fUIPaintState;

	ZUIButton* fButton_Arrow;
	ZUIButton* fButton_Text;
	//ZUIButton* fButton_Hiliter;
	ZUIButton* fButton_Dropper;
	ZUIButton* fButton_Fill;
	ZUIButton* fButton_Brush;
	ZUIButton* fButton_Rect;
	ZUIButton* fButton_Oval;
	ZUIButton* fButton_Polygon;
	ZUIButton* fButton_RectFrame;
	ZUIButton* fButton_OvalFrame;
	ZUIButton* fButton_PolyFrame;
	ZUIButton* fButton_Freeform;
	ZUIButton* fButton_Line;
	ZUIButton* fButton_Eraser;
	ZUIButton* fButton_Select;
	ZUIButton* fButton_Lasso;

	ZRef<ZUICaption> fCaption_StrokeFore;
	ZUIButton* fButton_StrokeFore;
	ZRef<ZUICaption> fCaption_StrokePattern;
	ZUIButton* fButton_StrokePattern;
	ZRef<ZUICaption> fCaption_StrokeBack;
	ZUIButton* fButton_StrokeBack;

	ZRef<ZUICaption> fCaption_FillFore;
	ZUIButton* fButton_FillFore;
	ZRef<ZUICaption> fCaption_FillPattern;
	ZUIButton* fButton_FillPattern;
	ZRef<ZUICaption> fCaption_FillBack;
	ZUIButton* fButton_FillBack;

	ZRef<ZUICaption> fCaption_PenShape;
	ZUIButton* fButton_PenShape;

	ZUIButton* GetHighlightedToolButton();
	ZUIButton* fHighlightedToolButton;
	};

// =================================================================================================
#pragma mark -
#pragma mark * NPainterStd_UI::ToolWindow

#if NPainterConfig_UseWindows

class ToolWindow : public ZWindow
	{
public:
	static ZOSWindow* sCreateOSWindow(ZApp* inApp);
	ToolWindow(ZApp* inApp, ZPoint inLocation, const ZRef<UIPaintState>& inUIPaintState, const ZAsset& inButtonAsset);
	virtual ~ToolWindow();

// From ZWindow
	virtual bool WindowQuitRequested();
	virtual void WindowCloseByUser();

protected:
	ZWindowPane* fWindowPane;
	ZSubPane* fToolPane;
	};

#endif // NPainterConfig_UseWindows


// =================================================================================================

} // namespace NPainterStd_UI

#endif // __NPainterStd_UI__

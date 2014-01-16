#include "PainterTest_Window.h"
#include "NPaintDataRep.h"

#include "ZAlert.h"
#include "ZApp.h"
#include "ZAsset.h"
#include "ZDCPixmapCoder_BMP.h"
#include "ZDCPixmapCoder_GIF.h"
#include "ZDCPixmapCoder_JPEGLib.h"
#include "ZDCPixmapCoder_PNG.h"
#include "ZMouseTracker.h"
#include "ZTicks.h"
#include "ZUIFactory.h"
#include "ZUtil_UI.h"

// =================================================================================================
#pragma mark -
#pragma mark PainterTest_PaintPane declaration

class PainterTest_PaintPane : public ZSubPane,
			public NPaintEngine::Host
	{
public:
	PainterTest_PaintPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const ZAsset& iCursorAsset, const ZRef<NPaintEngine::PaintState>& inPaintState);
	virtual ~PainterTest_PaintPane();

// From ZEventHr via ZSubPane
	virtual bool WantsToBecomeTarget();
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);
	virtual void DoSetupMenus(ZMenuSetup* inMenuSetup);
	virtual bool DoMenuMessage(const ZMessage& inMessage);

// From ZSubPane
	virtual void Activate(bool inIsNowActive);
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual void DoIdle();
	virtual void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);
	virtual bool GetCursor(ZPoint inPoint, ZCursor& outCursor);

	virtual ZPoint GetSize();

// From NPaintEngine::Host
	virtual ZSubPane* GetPaintEnginePane();
	virtual bool GetPaintEngineActive();
	virtual ZPoint GetPaintEngineLocation();
	virtual void InvalidatePaintEngineRgn(const ZDCRgn& inInvalRgn);
	virtual void InvalidatePaintEngineCursor();

	void SetPaintDataRep(ZRef<NPaintDataRep> iPaintDataRep);

	NPaintEngine* GetPaintEngine()
		{ return fPaintEngine; }

	void SetCropActive(bool iCropActive);

protected:
	ZAsset fCursorAsset;
	ZRef<NPaintEngine::PaintState> fPaintState;
	NPaintEngine* fPaintEngine;
	bool fCropActive;
	ZPoint fCropOffset;
	ZPoint fCropSize;

	void pDeltaCropOffset(ZPoint iDelta);
	void pSetCropSize(ZPoint iCropSize);

	class Tracker_Offset;
	friend class Tracker_Offset;

	class Tracker_Size;
	friend class Tracker_Size;
	};

// =================================================================================================
#pragma mark -
#pragma mark * PainterTest_PaintPane::Tracker_Offset

class PainterTest_PaintPane::Tracker_Offset : public ZMouseTracker
	{
public:
	Tracker_Offset(PainterTest_PaintPane* iPane, ZPoint inStartPoint, const ZCursor& iCursor);

// From ZCaptureInput via ZMouseTracker
	virtual bool GetCursor(ZPoint iPanePoint, ZCursor& oCursor);

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

protected:
	PainterTest_PaintPane* fPane;
	ZCursor fCursor;
	};

PainterTest_PaintPane::Tracker_Offset::Tracker_Offset(PainterTest_PaintPane* iPane, ZPoint inStartPoint, const ZCursor& iCursor)
:	ZMouseTracker(iPane, inStartPoint),
	fPane(iPane),
	fCursor(iCursor)
	{}

bool PainterTest_PaintPane::Tracker_Offset::GetCursor(ZPoint iPanePoint, ZCursor& oCursor)
	{
	oCursor = fCursor;
	return true;
	}

void PainterTest_PaintPane::Tracker_Offset::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	fPane->pDeltaCropOffset(inNext - inPrevious);
	}

// =================================================================================================
#pragma mark -
#pragma mark * PainterTest_PaintPane::Tracker_Size

class PainterTest_PaintPane::Tracker_Size : public ZMouseTracker
	{
public:
	Tracker_Size(PainterTest_PaintPane* iPane, ZPoint inStartPoint);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

protected:
	PainterTest_PaintPane* fPane;
	};

PainterTest_PaintPane::Tracker_Size::Tracker_Size(PainterTest_PaintPane* iPane, ZPoint inStartPoint)
:	ZMouseTracker(iPane, inStartPoint),
	fPane(iPane)
	{}

void PainterTest_PaintPane::Tracker_Size::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	fPane->pSetCropSize(inNext);
	}

// =================================================================================================
#pragma mark -
#pragma mark PainterTest_PaintPane definition

PainterTest_PaintPane::PainterTest_PaintPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const ZAsset& iCursorAsset, const ZRef<NPaintEngine::PaintState>& inPaintState)
:	ZSubPane(inSuperPane, inLocator),
	fCursorAsset(iCursorAsset),
	fPaintState(inPaintState),
	fPaintEngine(nil),
	fCropActive(false)
	{
	this->GetWindow()->RegisterIdlePane(this);
	fPaintEngine = new NPaintEngine(this, inPaintState, ZPoint(320, 240));
	}

PainterTest_PaintPane::~PainterTest_PaintPane()
	{
	this->GetWindow()->UnregisterIdlePane(this);
	delete fPaintEngine;
	}

void PainterTest_PaintPane::Activate(bool inIsNowActive)
	{
	if (fPaintEngine)
		fPaintEngine->Activate(inIsNowActive);
	}

void PainterTest_PaintPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZDC localDC = inDC;
	ZDCRgn oldClip = localDC.GetClip();

	const ZPoint paintEngineLocation = this->GetPaintEngineLocation();
	const ZPoint paintEngineSize = fPaintEngine->GetSize();

	const ZRect paintEngineRect = ZRect(paintEngineSize) + paintEngineLocation;
	localDC.SetClip(localDC.GetClip() & paintEngineRect);
	fPaintEngine->Draw(localDC, paintEngineLocation);
	localDC.SetClip(oldClip);

	if (fCropActive)
		{
		// Fill everything bar the painting with white
		localDC.SetInk(ZRGBColor::sWhite);
		localDC.Fill(inBoundsRgn - paintEngineRect);

		if ZCONFIG(OS, Win32)
			{
			localDC.SetInk(ZDCInk::sDkGray);
			localDC.SetMode(ZDC::modeOr);
			}
		else
			{
			localDC.SetInk(ZDCInk(ZRGBColor::sRed, ZRGBColor::sBlack, ZDCPattern::sDkGray));
			localDC.SetMode(ZDC::modeOr);
			}

		const ZRect theBounds = ZRect(paintEngineSize) + fCropOffset;
		localDC.Fill(ZDCRgn(theBounds) - ZRect(fCropSize));

		// Draw a 2 pixel black border along the bottom and
		// right edges of the crop rectangle.
		localDC.SetInk(ZRGBColor::sBlack);
		localDC.SetMode(ZDC::modeCopy);
		localDC.SetPenWidth(2);

		// Bottom edge
		localDC.Line(0, fCropSize.v, fCropSize.h, fCropSize.v);
		// Right edge
		localDC.Line(fCropSize.h, 0, fCropSize.h, fCropSize.v);		

		// Sizing rectangle
		localDC.Fill(ZRect(5,5) + fCropSize);
		}
	else
		{
		const ZDCRgn myRgn = inBoundsRgn - paintEngineRect;
		localDC.SetInk(ZRGBColor::sWhite / 2);
		localDC.Fill(myRgn);
		}
	}

bool PainterTest_PaintPane::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fCropActive)
		{
		if (inHitPoint.h < fCropSize.h && inHitPoint.v < fCropSize.v)
			{
			ZCursor theCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("HandClosed"));
			ZMouseTracker* theTracker = new Tracker_Offset(this, inHitPoint, theCursor);
			theTracker->Install();
			}
		else
			{
			ZMouseTracker* theTracker = new Tracker_Size(this, inHitPoint);
			theTracker->Install();
			}
		return true;
		}
	else
		{
		if (fPaintEngine)
			return fPaintEngine->Click(inHitPoint, inEvent);
		return false;
		}
	}

void PainterTest_PaintPane::DoIdle()
	{
	if (fCropActive)
		{
		}
	else
		{
		if (fPaintEngine)
			fPaintEngine->Idle(this->GetDC(), this->GetPaintEngineLocation());
		}
	}

void PainterTest_PaintPane::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	if (fCropActive)
		{
		this->InvalidateCursor();
		}
	else
		{
		if (fPaintEngine)
			fPaintEngine->MouseMotion(inPoint, inMotion);
		}
	}

bool PainterTest_PaintPane::GetCursor(ZPoint inPoint, ZCursor& outCursor)
	{
	if (fCropActive)
		{
		if (inPoint.h < fCropSize.h && inPoint.v < fCropSize.v)
			{
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Hand"));
			}
		else
			{
			outCursor = ZUtil_UI::sGetCursor(fCursorAsset.GetChild("Crop"));
			}
		return true;
		}
	else
		{
		if (fPaintEngine)
			return fPaintEngine->GetCursor(inPoint, outCursor);
		return false;
		}
	}

bool PainterTest_PaintPane::WantsToBecomeTarget()
	{ return true; }

bool PainterTest_PaintPane::DoKeyDown(const ZEvent_Key& inEvent)
	{
	if (fCropActive)
		{
		return false;
		}
	else
		{
		if (fPaintEngine)
			return fPaintEngine->KeyDown(inEvent);
		return false;
		}
	}

void PainterTest_PaintPane::DoSetupMenus(ZMenuSetup* inMenuSetup)
	{
	if (!fCropActive)
		{
		if (fPaintEngine)
			fPaintEngine->SetupMenus(inMenuSetup);
		}

	inMenuSetup->EnableDisableCheckMenuItem(mcUser + 1, true, fCropActive);
	}

bool PainterTest_PaintPane::DoMenuMessage(const ZMessage& inMessage)
	{
	switch (inMessage.GetInt32("menuCommand"))
		{
		case mcUser + 1:
			{
			this->SetCropActive(!fCropActive);
			return true;
			}
		}

	if (fCropActive)
		{
		return false;
		}
	else
		{
		if (fPaintEngine)
			return fPaintEngine->MenuMessage(inMessage);
		}
	return false;
	}

ZPoint PainterTest_PaintPane::GetSize()
	{
	const ZPoint paintEngineLocation = this->GetPaintEngineLocation();
	const ZPoint paintEngineSize = fPaintEngine ? fPaintEngine->GetSize() : ZPoint::sZero;

	ZRect myRect = ZRect(paintEngineSize) + paintEngineLocation;
	if (fCropActive)
		{
		// Include the boundary of the crop rectangle, plus its border and sizing handle.
		myRect |= ZRect(fCropSize + ZPoint(5,5));
		}
	return myRect.BottomRight();
	}

ZSubPane* PainterTest_PaintPane::GetPaintEnginePane()
	{ return this; }

bool PainterTest_PaintPane::GetPaintEngineActive()
	{ return this->GetActive(); }

ZPoint PainterTest_PaintPane::GetPaintEngineLocation()
	{
	if (fCropActive)
		return fCropOffset;
	return ZPoint::sZero;
	}

void PainterTest_PaintPane::InvalidatePaintEngineRgn(const ZDCRgn& inInvalRgn)
	{
	ZDCRgn theRgn = inInvalRgn + this->GetPaintEngineLocation();
	this->Invalidate(theRgn);
	}

void PainterTest_PaintPane::InvalidatePaintEngineCursor()
	{
	this->InvalidateCursor();
	}

void PainterTest_PaintPane::SetPaintDataRep(ZRef<NPaintDataRep> iPaintDataRep)
	{
	this->Refresh();
	delete fPaintEngine;
	fPaintEngine = nil;
	if (iPaintDataRep && iPaintDataRep->HasData())
		fPaintEngine = new NPaintEngine(this, fPaintState, iPaintDataRep);	
	this->Refresh();
	}

void PainterTest_PaintPane::SetCropActive(bool iCropActive)
	{
	if (fCropActive == iCropActive)
		return;

	ZPoint engineSize = ZPoint::sZero;
	if (fPaintEngine)
		engineSize = fPaintEngine->GetSize();

	if (fCropActive)
		{
		if (engineSize != fCropSize || fCropOffset)
			{
			if (fPaintEngine)
				fPaintEngine->Resize(fCropOffset, fCropSize);
			}
		fCropActive = false;
		}
	else
		{
		fCropOffset = ZPoint(0, 0);
		fCropSize = engineSize;
		fCropActive = true;
		}

	this->InvalidateCursor();
	this->GetOutermostSuperPane()->Refresh();
	}

void PainterTest_PaintPane::pDeltaCropOffset(ZPoint iDelta)
	{
	if (iDelta.h || iDelta.v)
		{
		fCropOffset += iDelta;
		this->GetOutermostSuperPane()->Refresh();		
		}
	}

void PainterTest_PaintPane::pSetCropSize(ZPoint iCropSize)
	{
	if (fCropSize != iCropSize)
		{
		fCropSize = iCropSize;
		this->GetOutermostSuperPane()->Refresh();		
		}
	}

// ==================================================
#pragma mark -
#pragma mark PainterTest_Window

ZOSWindow* PainterTest_Window::sCreateOSWindow(ZApp* inApp)
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0, 0, 630, 400);
	theAttributes.fLook = ZOSWindow::lookDocument;
	theAttributes.fLayer = ZOSWindow::layerDocument;
	theAttributes.fResizable = true;
	theAttributes.fHasSizeBox = true;
	theAttributes.fHasCloseBox = true;
	theAttributes.fHasZoomBox = true;
	theAttributes.fHasMenuBar = true;
	theAttributes.fHasTitleIcon = true;

	return inApp->CreateOSWindow(theAttributes);
	}

PainterTest_Window::PainterTest_Window(ZApp* inApp, const ZAsset& iAsset, ZRef<NPaintEngine::PaintState> inPaintState, ZRef<NPaintDataRep> iPaintDataRep, const ZFileSpec& iFileSpec, const string& iFormat)
:	ZWindow(inApp, sCreateOSWindow(inApp)),
	ZPaneLocator(nil),
	fFileSpec(iFileSpec),
	fFormat(iFormat)
	{
	fWindowPane = new ZWindowPane(this, this);
	fScrollPane = new ZUIScrollPane(fWindowPane, this);
	fScrollBar_H = ZUIFactory::sGet()->Make_ScrollBar(fWindowPane, nil);
	new ZUIScrollableScrollBarConnector(fScrollBar_H, fScrollPane, fScrollPane, true);

	fScrollBar_V = ZUIFactory::sGet()->Make_ScrollBar(fWindowPane, nil);
	new ZUIScrollableScrollBarConnector(fScrollBar_V, fScrollPane, fScrollPane, false);
	
	fPaintPane = new PainterTest_PaintPane(fScrollPane, this, iAsset.GetChild("Cursors"), inPaintState);
	if (iPaintDataRep)
		fPaintPane->SetPaintDataRep(iPaintDataRep);
	fPaintPane->BecomeWindowTarget();

	if (fFileSpec)
		this->SetTitle(fFileSpec.Name());
	else
		this->SetTitle("Untitled Painting");

	fChangeCount = fPaintPane->GetPaintEngine()->GetChangeCount();
	}

PainterTest_Window::~PainterTest_Window()
	{}

bool PainterTest_Window::GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk)
	{
	if (inPane == fWindowPane)
		{
		outInk = ZRGBColor::sWhite - (ZRGBColor::sWhite / 4);
		return true;
		}
	return ZPaneLocator::GetPaneBackInk(inPane, inDC, outInk);
	}

bool PainterTest_Window::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

bool PainterTest_Window::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
//	if (inPane == fPaintPane)
//		{
//		outLocation = ZPoint(15, 20);
//		return true;
//		}
	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool PainterTest_Window::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (inPane == fScrollPane)
		{
		outSize = fWindowPane->GetInternalSize();
		outSize.h -= fScrollBar_V->GetPreferredThickness();
		outSize.v -= fScrollBar_H->GetPreferredThickness();
		return true;
		}
	return ZPaneLocator::GetPaneSize(inPane, outSize);
	}

void PainterTest_Window::DoSetupMenus(ZMenuSetup* inMenuSetup)
	{
	if (fFileSpec && fChangeCount != fPaintPane->GetPaintEngine()->GetChangeCount())
		{
		inMenuSetup->EnableItem(mcSave);
		}
	inMenuSetup->EnableItem(mcClose);
	return ZWindow::DoSetupMenus(inMenuSetup);
	}

bool PainterTest_Window::DoMenuMessage(const ZMessage& inMessage)
	{
	switch (inMessage.GetInt32("menuCommand"))
		{
		case mcSave:
			{
			this->pDoSave();
			return true;
			}
		case mcClose:
			{
			this->pDoClose();
			return true;
			}
		}
	
	return ZWindow::DoMenuMessage(inMessage);
	}

bool PainterTest_Window::WindowQuitRequested()
	{
	return this->pDoClose();
	}

void PainterTest_Window::WindowCloseByUser()
	{
	this->pDoClose();
	}

bool PainterTest_Window::pDoClose()
	{
	// if painting has been changed, but not saved, then prompt user with:
	// 'Do you want to save the changes to "<filename>"?'
	// "Don't Save", "Cancel" and "Save" options.
	if (fFileSpec && fChangeCount != fPaintPane->GetPaintEngine()->GetChangeCount())
		{
		ZAlert::Result result =
			ZAlert::sSaveChanges("",
			"Do you want to save the changes to \"" + fFileSpec.Name() + "\"?",
			"");

		if (result == ZAlert::eResultCancel)
			{
			// Don't save, don't close.
			return false;
			}

		if (result == ZAlert::eResultOK)
			{
 			// Save and then close.
			this->pDoSave();
 			}
		// must be eResultOK: Don't save, do close.
		}
	this->CloseAndDispose();
	return true;
	}

static void sWrite(ZRef<NPaintDataRep> iDataRep, const string& iFormat, const ZStreamWPos& iStreamWPos)
	{
	ZDCPixmapEncoder* theEncoder = nil;
	if (false)
		{}
	#if ZCONFIG_SPI_Enabled(jpeglib)
	else if (iFormat == "jpg")
		{
		theEncoder = new ZDCPixmapEncoder_JPEGLib(100);
		}
	#endif
	#if ZCONFIG_SPI_Enabled(libpng)
	else if (iFormat == "png")
		{
		theEncoder = new ZDCPixmapEncoder_PNG;
		}
	#endif

	if (theEncoder)
		{
		theEncoder->Write(iStreamWPos, iDataRep->GetPixmap());
		}
	else
		{
		iDataRep->ToStream(iStreamWPos);
		}

	iStreamWPos.Truncate();
	}
	
void PainterTest_Window::pDoSave()
	{
	fPaintPane->SetCropActive(false);
	try
		{
		if (ZRef<NPaintDataRep> theDataRep = fPaintPane->GetPaintEngine()->GetPaintData())
			{
			if (ZRef<ZStreamerWPos> theStreamerWPos = fFileSpec.CreateWPos(true, true))
				{
				sWrite(theDataRep, fFormat, theStreamerWPos->GetStreamWPos());
			
				fChangeCount = fPaintPane->GetPaintEngine()->GetChangeCount();
				}
			}
		}
	catch (...)
		{}
	}


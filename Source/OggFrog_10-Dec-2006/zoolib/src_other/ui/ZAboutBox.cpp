static const char rcsid[] = "@(#) $Id: ZAboutBox.cpp,v 1.3 2004/05/05 15:55:03 agreen Exp $";

#include "ZAboutBox.h"
#include "ZApp.h" // For CreateOSWindow and sGet

class ZAboutBox::PixmapPane : public ZSubPane
	{
public:
	PixmapPane(ZSuperPane* inSuperPane, ZWindow* inWindow, const ZDCPixmap& inPixmap);
	virtual ~PixmapPane();

	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual ZPoint GetSize();

protected:
	ZWindow* fWindow;
	ZDCPixmap fPixmap;
	};

ZAboutBox::PixmapPane::PixmapPane(ZSuperPane* inSuperPane, ZWindow* inWindow, const ZDCPixmap& inPixmap)
:	ZSubPane(inSuperPane, nil), fWindow(inWindow), fPixmap(inPixmap)
	{}

ZAboutBox::PixmapPane::~PixmapPane()
	{}

void ZAboutBox::PixmapPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	inDC.Draw(ZPoint::sZero, fPixmap);
	}

bool ZAboutBox::PixmapPane::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	fWindow->CloseAndDispose();
	return true;
	}

ZPoint ZAboutBox::PixmapPane::GetSize()
	{
	return fPixmap.Size();
	}

static ZOSWindow* sCreateOSWindow()
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0,0,100,100);
	theAttributes.fLook = ZOSWindow::lookThinBorder;
	theAttributes.fLayer = ZOSWindow::layerDialog;
	theAttributes.fResizable = false;
	theAttributes.fHasSizeBox = false;
	theAttributes.fHasCloseBox = false;
	theAttributes.fHasZoomBox = false;
	theAttributes.fHasMenuBar = false;
	theAttributes.fHasTitleIcon = false;

	return ZApp::sGet()->CreateOSWindow(theAttributes);
	}

ZAboutBox::ZAboutBox(const ZDCPixmap& inPixmap)
:	ZWindow(ZApp::sGet(), sCreateOSWindow())
	{
	ZWindowPane* theWindowPane = new ZWindowPane(this, nil);
	ZSubPane* theSubPane = new PixmapPane(theWindowPane, this, inPixmap);
	this->SetSizeLimits(theSubPane->GetSize(), theSubPane->GetSize());
	this->SetSize(theSubPane->GetSize());
	this->CenterDialog();
	this->BringFront(true);
	this->GetWindowLock().Release();
	}

ZAboutBox::~ZAboutBox()
	{}

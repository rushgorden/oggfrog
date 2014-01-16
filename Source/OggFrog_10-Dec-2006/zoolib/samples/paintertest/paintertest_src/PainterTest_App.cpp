#include "PainterTest_App.h"
#include "PainterTest_MenuDef.h"
#include "PainterTest_Window.h"
#include "NPaintDataRep.h"
#include "NPainterStd_UI.h"

#include "ZFontMenus.h"
#include "ZLog.h"
#include "ZMain.h"
#include "ZMenu.h"
#include "ZString.h"
#include "ZStream_Buffered.h"
#include "ZTicks.h"
#include "ZTypes.h"

#include "ZUtil_UI.h"
#include "ZUtil_Asset.h"

#include "ZAlert.h"

#define TESTING 0//(!ZCONFIG(OS, Win32))

// ====================================================================================================
#pragma mark -
#pragma mark * Helper functions

static bool sTryBMP(const ZStreamR& iStream)
	{
	if (0x42 != iStream.ReadUInt8() || 0x4D != iStream.ReadUInt8())
		{
		return false;
		}
	return true;
	}

static bool sTryGIF(const ZStreamR& iStream)
	{
	if ('G' != iStream.ReadUInt8() || 'I' != iStream.ReadUInt8() || 'F' != iStream.ReadUInt8() || '8' != iStream.ReadUInt8())
		{
		return false;
		}
	return true;
	}

static bool sTryPNG(const ZStreamR& iStream)
	{
	if (0x89 != iStream.ReadUInt8() || 0x50 != iStream.ReadUInt8() || 0x4E != iStream.ReadUInt8() || 0x47 != iStream.ReadUInt8()
		|| 0x0D != iStream.ReadUInt8() || 0x0A != iStream.ReadUInt8() || 0x1A != iStream.ReadUInt8() || 0x0A != iStream.ReadUInt8())
		{
		return false;
		}
	return true;
	}

static bool sTryKF(const ZStreamR& iStream)
	{
	iStream.Skip(8);
	if ('P' != iStream.ReadUInt8() || 'N' != iStream.ReadUInt8() || 'T' != iStream.ReadUInt8() || 'R' != iStream.ReadUInt8())
		{
		return false;
		}
	return true;
	}

static void sSniffFile(ZStreamR_DynamicBuffered& iStream)
	{
	if (sTryBMP(iStream))
		return;
	iStream.Rewind();

	if (sTryGIF(iStream))
		return;
	iStream.Rewind();

//	if (sTryJPEG(iStream))
//		return;
//	iStream.Rewind();

	if (sTryPNG(iStream))
		return;
	iStream.Rewind();

	if (sTryKF(iStream))
		return;
	iStream.Rewind();
	}

#include "ZDCPixmapCoder_BMP.h"
#include "ZDCPixmapCoder_GIF.h"
#include "ZDCPixmapCoder_JPEGLib.h"
#include "ZDCPixmapCoder_PNG.h"

static ZRef<NPaintDataRep> sTryRead_NPaintDataRep(const ZStreamR& iStreamR, ZDCPixmapDecoder* iDecoder)
	{
	ZRef<NPaintDataRep> theRep;
	try
		{
		if (ZDCPixmap thePixmap = ZDCPixmapDecoder::sReadPixmap(iStreamR, *iDecoder))
			theRep = new NPaintDataRep(thePixmap);
		}
	catch (...)
		{}

	delete iDecoder;
	return theRep;
	}

template <class D>
ZRef<NPaintDataRep> sTryRead_T(const ZStreamR& iStreamR)
	{
	ZStreamR_Buffered theSRB(4096, iStreamR);
	return sTryRead_NPaintDataRep(theSRB, new D);
	}

static ZRef<NPaintDataRep> sTryRead_BMP(const ZStreamR& iStreamR)
	{
	ZStreamR_Buffered theSRB(4096, iStreamR);
	return sTryRead_NPaintDataRep(theSRB, new ZDCPixmapDecoder_BMP(true));
	}

static ZRef<NPaintDataRep> sTryRead_GIF(const ZStreamR& iStreamR)
	{
	ZStreamR_Buffered theSRB(4096, iStreamR);
	return sTryRead_NPaintDataRep(theSRB, new ZDCPixmapDecoder_GIF);
	}

static ZRef<NPaintDataRep> sTryRead_JPEG(const ZStreamR& iStreamR)
	{
	ZStreamR_Buffered theSRB(4096, iStreamR);
	#if ZCONFIG_SPI_Enabled(JPEGLib)
		return sTryRead_NPaintDataRep(theSRB, new ZDCPixmapDecoder_JPEGLib);
	#else
		return ZRef<NPaintDataRep>();
	#endif
	}

static ZRef<NPaintDataRep> sTryRead_PNG(const ZStreamR& iStreamR)
	{
	ZStreamR_Buffered theSRB(4096, iStreamR);
	#if ZCONFIG_SPI_Enabled(libpng)
		return sTryRead_NPaintDataRep(theSRB, new ZDCPixmapDecoder_PNG);
	#else
		return ZRef<NPaintDataRep>();
	#endif
	}

static ZRef<NPaintDataRep> sRead_NPaintDataRep(const ZStreamRPos& iStreamRPos, string& oFormat)
	{
	iStreamRPos.SetPosition(0);
	if (ZRef<NPaintDataRep> theRep = sTryRead_BMP(iStreamRPos))
		{
		oFormat = "bmp";
		return theRep;
		}

	iStreamRPos.SetPosition(0);
	if (ZRef<NPaintDataRep> theRep = sTryRead_GIF(iStreamRPos))
		{
		oFormat = "gif";
		return theRep;
		}

	iStreamRPos.SetPosition(0);
	if (ZRef<NPaintDataRep> theRep = sTryRead_JPEG(iStreamRPos))
		{
		oFormat = "jpg";
		return theRep;
		}

	iStreamRPos.SetPosition(0);
	if (ZRef<NPaintDataRep> theRep = sTryRead_PNG(iStreamRPos))
		{
		oFormat = "png";
		return theRep;
		}

	iStreamRPos.SetPosition(0);
	ZRef<NPaintDataRep> theRep = new NPaintDataRep;
	theRep->FromStream(iStreamRPos);
	if (theRep->HasData())
		{
		oFormat = "kf";
		return theRep;
		}

	return new NPaintDataRep(ZPoint(320, 240));
	}

// ====================================================================================================
#pragma mark -
#pragma mark * PainterTest_App

#include "ZAsset_Std.h"
#include "ZStreamRWPos_RAM.h"
#include "ZStream_Win.h"
#include "ZUtil_Win.h"

// On windows we're transcribing the whole of any resource based asset tree into a RAM-based
// streamer, so that when we're running across a high-latency filesystem we're not affected as much.
static ZAsset sGetAssetRootFromExecutable(const string& iAssetTreeName)
	{
	#if !ZCONFIG(OS, Win32)

		return ZUtil_Asset::sGetAssetRootFromExecutable(iAssetTreeName);

	#else
		HINSTANCE theHINSTANCE;
		if (ZUtil_Win::sUseWAPI())
			theHINSTANCE = ::GetModuleHandleW(nil);
		else
			theHINSTANCE = ::GetModuleHandleA(nil);

		ZRef<ZStreamerRWPos> theStreamer = new ZStreamerRWPos_T<ZStreamRWPos_RAM>;
		theStreamer->GetStreamWPos().CopyAllFrom(ZStreamRPos_Win_MultiResource(theHINSTANCE, "ZAO_", iAssetTreeName));
		theStreamer->GetStreamWPos().SetPosition(0);

		ZRef<ZAssetTree> theAssetTree = new ZAssetTree_Std_Streamer(theStreamer);
		return theAssetTree->GetRoot();
	#endif

	return ZAsset();
	}

// ====================================================================================================
#pragma mark -
#pragma mark * PainterTest_App

PainterTest_App* PainterTest_App::sPainterTest_App;

PainterTest_App::PainterTest_App()
	{
	fAsset = sGetAssetRootFromExecutable("PainterTest_Assets_en").GetChild("en|PainterTest");
	fAsset_NPainterStd = sGetAssetRootFromExecutable("NPainterStd_Assets_en").GetChild("en|NPainterStd");

	// Grab the paint state's palette data

	// Monochrome patterns
	vector<ZDCPattern> monoPatterns;
	NPaintEngine::sExtractMonoPatterns(ZUtil_UI::sGetPixmap(fAsset_NPainterStd.GetChild("MonoPatterns")), monoPatterns);

	// The color table
	vector<ZRGBColor> colorTable;
#if 1
	NPaintEngine::sExtractColors(ZUtil_UI::sGetPixmap(fAsset_NPainterStd.GetChild("ColorTable256")), 16, 16, colorTable);
#else
	// This builds a 6 x 6 x 6 color cube, which is not very intuitive when squashed onto a 2D grid
	ZRGBColor theRGBColor;
	theRGBColor.alpha = 0xFFFFU;
	for (size_t blue = 0; blue < 6; ++blue)
		{
		for (size_t green = 0; green < 6; ++green)
			{
			for (size_t red = 0; red < 6; ++red)
				{
				theRGBColor.red = red * 0x3333U;
				theRGBColor.green = green * 0x3333U;
				theRGBColor.blue = blue * 0x3333U;
				colorTable.push_back(theRGBColor);
				}
			}
		}
#endif
	// Pixmap patterns
	vector<ZDCPixmap> pixmapPatterns;
	for (ZAssetIter theIter = fAsset.GetChild("Textures"); theIter; theIter.Advance())
		pixmapPatterns.push_back(ZUtil_UI::sGetPixmap(theIter.Current()));

	// Pen Shapes
	vector<ZDCRgn> penShapes;
	penShapes.push_back(ZDCRgn::sEllipse(0, 0, 1, 1));
	penShapes.push_back(ZDCRgn::sEllipse(-1, -1, 1, 1));
	penShapes.push_back(ZDCRgn::sEllipse(-2, -2, 2, 2));
	penShapes.push_back(ZDCRgn::sEllipse(-4, -4, 4, 4));
	penShapes.push_back(ZDCRgn::sEllipse(-8, -8, 8, 8));
	penShapes.push_back(ZDCRgn::sEllipse(-12, -12, 12, 12));

	penShapes.push_back(ZRect(0, 0, 1, 1));
	penShapes.push_back(ZRect(-1, -1, 1, 1));
	penShapes.push_back(ZRect(-2, -2, 2, 2));
	penShapes.push_back(ZRect(-4, -4, 4, 4));
	penShapes.push_back(ZRect(-8, -8, 8, 8));
	penShapes.push_back(ZRect(-12, -12, 12, 12));

	// Also add in a complex shape, just for kicks
//	ZDCRgn theRgn = ZDCRgn::sEllipse(-12, -12, 0, 0) | ZDCRgn::sEllipse(0, 0, 12, 12);
//	penShapes.push_back(theRgn);

	// Instantiate our paint state
	fPaintState = new NPainterStd_UI::UIPaintState(fAsset_NPainterStd.GetChild("Cursors"), monoPatterns, colorTable, pixmapPatterns, penShapes);

	sPainterTest_App = this;
	}

PainterTest_App::~PainterTest_App()
	{
	ZAssert(sPainterTest_App == this);
	sPainterTest_App = nil;
	}

void PainterTest_App::WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall)
	{
	if (ZRef<ZMenu> appleMenu = inMenuInstall.GetAppleMenu())
		{
		appleMenu->RemoveAll();
		appleMenu->Append(mcAbout, "About " + this->GetAppName() + "...");
		}

	// Build the top level menus
	ZRef<ZMenu> fileMenu = new ZMenu;
	inMenuInstall.Append("&File", fileMenu);
#if TESTING
		fileMenu->Append(mcNew, "&New...", 'N');
		fileMenu->Append(mcOpen, "&Open...", 'O');
		fileMenu->AppendSeparator();
#endif
		fileMenu->Append(mcClose, "&Close...", 'W');
		fileMenu->AppendSeparator();
		fileMenu->Append(mcSave, "Save", 'S');
//		fileMenu->AppendSeparator();
//		fileMenu->Append(mcPageSetup, "P&age Setup...");
//		fileMenu->Append(mcPrint, "&Print...", 'P');
		if (!this->HasGlobalMenuBar())
			{
			fileMenu->AppendSeparator();
			fileMenu->Append(mcQuit, "&Quit", 'Q');
			}

	ZRef<ZMenu> editMenu = new ZMenu;
	inMenuInstall.Append("&Edit", editMenu);
		editMenu->Append(mcUndo, "&Undo", 'Z');
		editMenu->AppendSeparator();
		editMenu->Append(mcCut, "Cu&t", 'X');
		editMenu->Append(mcCopy, "&Copy", 'C');
		editMenu->Append(mcPaste, "&Paste", 'V');
		editMenu->Append(mcClear,	 "Clea&r");
		editMenu->AppendSeparator();
		editMenu->Append(mcSelectAll, "Select &All", 'A');
		editMenu->AppendSeparator();
		editMenu->Append(mcUser + 1, "Resize", 'R');

	ZRef<ZMenu> textMenu = new ZMenu;
	inMenuInstall.Append("Text", textMenu);
		textMenu->Append("Font", ZFontMenus::sMakeFontMenu());
		textMenu->Append("Size", ZFontMenus::sMakeSizeMenu());
		textMenu->Append("Style", ZFontMenus::sMakeStyleMenu());
	}

void PainterTest_App::WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup)
	{
	ZApp::WindowSupervisorSetupMenus(inMenuSetup);

	inMenuSetup.EnableItem(mcNew);
	}

void PainterTest_App::ReceivedMessage(const ZMessage& inMessage)
	{
	bool handledIt = false;
	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Menu")
			{
			switch (inMessage.GetInt32("menuCommand"))
				{
				case mcNew:
					{
					handledIt = true;
					PainterTest_Window* theWindow = new PainterTest_Window(this, fAsset.GetChild("Windows").GetChild("Painter"), fPaintState, ZRef<NPaintDataRep>(), ZFileSpec(), string());
					theWindow->Center();
					theWindow->BringFront();
					theWindow->GetWindowLock().Release();
					break;
					}
				}
			}
		}
	if (!handledIt)
		ZApp::ReceivedMessage(inMessage);
	}

void PainterTest_App::OpenDocument(const ZMessage& inMessage)
	{
	const vector<ZTupleValue>& theVector = inMessage.GetVector("files");
	for (vector<ZTupleValue>::const_iterator i= theVector.begin(); i != theVector.end(); ++i)
		{
		if (ZFileSpec theFileSpec = ZRefDynamicCast<ZFileLoc>((*i).GetRefCounted()))
			this->OpenFileSpec(theFileSpec);
		}
	}

void PainterTest_App::RunStarted()
	{
	ZApp::RunStarted();

	ZWindow* theWindow;

	theWindow = new NPainterStd_UI::ToolWindow(this, ZPoint::sZero, fPaintState, fAsset_NPainterStd.GetChild("Buttons"));
	theWindow->SetLocation(theWindow->GetOSWindow()->GetContentTopLeftInset() + theWindow->GetOSWindow()->GetMainScreenFrame(true).TopLeft());
	theWindow->BringFront();
	theWindow->GetWindowLock().Release();

	if (TESTING)
		{
		theWindow = new PainterTest_Window(this, fAsset.GetChild("Windows").GetChild("Painter"), fPaintState, ZRef<NPaintDataRep>(), ZFileSpec(), string());
		theWindow->Center();
		theWindow->BringFront();
		theWindow->GetWindowLock().Release();
		}
	}

void PainterTest_App::WindowRosterChanged()
	{
	auto_ptr<ZOSWindowRoster> theRoster(fOSApp->CreateOSWindowRoster());
	for (;;)
		{
		ZOSWindow* theOSWindow;
		if (!theRoster->GetNextOSWindow(2000000, theOSWindow))
			{
			// Timed out, just try again. This is a bad thing
			// if the window is deadlocked against our lock, but
			// that shouldn't be the case, and in fact the timeout
			// shouldn't happen either.
			continue;
			}

		if (!theOSWindow)
			{
			// No more windows, drop out of the loop.
			break;
			}

		if (PainterTest_Window* theWindow = dynamic_cast<PainterTest_Window*>(theOSWindow->GetOwner()))
			{
			// We've got a painter window, so just return.
			theRoster->UnlockCurrentOSWindow();
			return;
			}
		theRoster->UnlockCurrentOSWindow();
		}

	// There are no open painter windows, so quit.
	if (!TESTING && this->QuitRequested())
		fOSApp->ExitRun();
	}

// Utility function in place of ZFileSpec::operator==(const ZFileSpec& iOther).
static bool sFileSpec_Equal(const ZFileSpec& iFS1, const ZFileSpec& iFS2)
	{
	return ZUnicode::sToLower(iFS1.AsString()) == ZUnicode::sToLower(iFS2.AsString());
	}

PainterTest_Window* PainterTest_App::GetPainterWindow(const ZFileSpec& inFileSpec)
	{
	auto_ptr<ZOSWindowRoster> theRoster(fOSApp->CreateOSWindowRoster());
	bool allOkay = true;
	while (allOkay)
		{
		bool askedAnyWindows = false;
		while (allOkay)
			{
			ZOSWindow* theOSWindow;
			if (theRoster->GetNextOSWindow(2000000, theOSWindow))
				{
				if (!theOSWindow)
					break;
				if (PainterTest_Window* theWindow = dynamic_cast<PainterTest_Window*>(theOSWindow->GetOwner()))
					{
					askedAnyWindows = true;
					if (sFileSpec_Equal(inFileSpec, theWindow->GetFileSpec()))
						return theWindow;
					}
				theRoster->UnlockCurrentOSWindow();
				}
			else
				{
				// If we timed out, treat it as if the user had explicitly cancelled.
				allOkay = false;
				}
			}
		if (!askedAnyWindows)
			break;
		}
	return nil;
	}

void PainterTest_App::OpenFileSpec(const ZFileSpec& inFileSpec)
	{
#if 0 // debugging. -ec 06.02.23
	ZAlert::sCautionAlert("Debugging OpenFileSpec()",
		"inFileSpec = '" + inFileSpec.AsString() + "'",
		"blah blah blah",
		60 * ZTicks::sPerSecond());
#endif

	// in this scenario, if inFileSpec has a PainterTest_Window that is already open for it, then simply bring that window to the front.
	// note that if the file described by inFileSpec has been modified, those modifications will *not* be reflected in window Ð and will
	// be lost if window is saved. -ec 06.02.21
	if (PainterTest_Window* theWindow = this->GetPainterWindow(inFileSpec))
		{
		theWindow->BringFront();
		theWindow->GetWindowLock().Release();
		return;
		}

	if (ZRef<ZStreamerRPos> theStreamerRPos = inFileSpec.OpenRPos(false))
		{
		string format;
		if (ZRef<NPaintDataRep> thePaintDataRep = sRead_NPaintDataRep(theStreamerRPos->GetStreamRPos(), format))
			{
			PainterTest_Window* theWindow = new PainterTest_Window(this, fAsset.GetChild("Windows").GetChild("Painter"), fPaintState, thePaintDataRep, inFileSpec, format);
			theWindow->Center();
			theWindow->BringFront();
			theWindow->GetWindowLock().Release();
			}
		}
	}

static const char rcsid[] = "@(#) $Id: ZOSWindow_Mac.cpp,v 1.27 2006/07/23 22:07:26 agreen Exp $";

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

#include "ZOSWindow_Mac.h"

#if ZCONFIG(API_OSWindow, Mac)

#include "ZDC_QD.h"
#include "ZDragClip.h"
#include "ZMain.h"
#include "ZMemory.h"
#include "ZMenu_Mac.h"
#include "ZString.h"
#include "ZThreadSimple.h"
#include "ZTicks.h"
#include "ZUtil_Mac_HL.h"
#include "ZUtil_Mac_LL.h"
#include "ZUtil_STL.h"

#include <AERegistry.h>
#include <Appearance.h>
#include <AppleEvents.h>
#include <Balloons.h>
#include <Displays.h>
#include <DiskInit.h>
#include <Drag.h>
#include <Gestalt.h>
#include <MacWindows.h>
#include <Processes.h>
#include <QDOffscreen.h>
#include <Resources.h>
#include <Sound.h> // For SysBeep

#define GetPortBitMapForCopyBits(a) &(((GrafPtr)a)->portBits)

#if !defined(UNIVERSAL_INTERFACES_VERSION) || (UNIVERSAL_INTERFACES_VERSION < 0x0320)
#	error We now *require* UNIVERSAL_INTERFACES_VERSION >= 0x0320
#endif

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// ==================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_Mac 1

// =================================================================================================
#pragma mark -
#pragma mark * Utility Methods

// We compile with STRICT_WINDOWS switched on. But in a couple of places we need access to the guts of a
// window, so we declare our own private version of WindowRecord and WindowPeek. One day Apple will quit
// fucking around and decide whether a window is a WindowRef, a WindowRecord* or whatever.

#include <LowMem.h>
#define GetWindowList LMGetWindowList

struct WindowRecord_;
typedef WindowRecord_* WindowPeek_;

#if PRAGMA_ALIGN_SUPPORTED
#	pragma options align=mac68k
#endif
struct WindowRecord_
	{
	GrafPort port;
	short windowKind;
	Boolean visible;
	Boolean hilited;
	Boolean goAwayFlag;
	Boolean spareFlag;
	RgnHandle strucRgn;
	RgnHandle contRgn;
	RgnHandle updateRgn;
	Handle windowDefProc;
	Handle dataHandle;
	StringHandle titleHandle;
	short titleWidth;
	Handle controlList;
	WindowPeek_ nextWindow;
	PicHandle windowPic;
	long refCon;
	};
#if PRAGMA_ALIGN_SUPPORTED
#	pragma options align=reset
#endif

static bool sGestalt_Appearance()
	{
	long response;
	OSErr err = ::Gestalt(gestaltAppearanceAttr, &response);
	if (err == noErr)
		return ((response & (1 << gestaltAppearanceExists)) != 0);
	return false;
	}

static bool sGestalt_Appearance101()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltAppearanceAttr, &response);
		if (err == noErr)
			{
			if ((response & (1 << gestaltAppearanceExists)) != 0)
				{
				err = ::Gestalt(gestaltAppearanceVersion, &response);
				if (err == noErr && response >= 0x0101)
					result = true;
				}
			}
		checked = true;
		}
	return result;
	}

static bool sGestalt_Appearance11()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltAppearanceAttr, &response);
		if (err == noErr)
			{
			if ((response & (1 << gestaltAppearanceExists)) != 0)
				{
				err = ::Gestalt(gestaltAppearanceVersion, &response);
				if (err == noErr && response >= 0x0110)
					result = true;
				}
			}
		checked = true;
		}
	return result;
	}

static bool sGestalt_Appearance111()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltAppearanceAttr, &response);
		if (err == noErr)
			{
			if ((response & (1 << gestaltAppearanceExists)) != 0)
				{
				err = ::Gestalt(gestaltAppearanceVersion, &response);
				if (err == noErr && response >= 0x0111)
					result = true;
				}
			}
		checked = true;
		}
	return result;
	}

static bool sGestalt_WindowManager()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltWindowMgrAttr, &response);
		if (err == noErr)
			result = ((response & gestaltWindowMgrPresent) != 0);
		checked = true;
		}
	return result;
	}

static bool sGestalt_IconServices()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltIconUtilitiesAttr, &response);
		if (err == noErr)
			result = ((response & (1 << gestaltIconUtilitiesHasIconServices)) != 0);
		checked = true;
		}
	return result;
	}

static bool sGestalt_OS86OrLater()
	{
	static bool checked = false;
	static bool result = false;
	if (!checked)
		{
		long response;
		OSErr err = ::Gestalt(gestaltSystemVersion, &response);
		if (err == noErr)
			result = response >= 0x0860;
		checked = true;
		}
	return result;
	}

// ==================================================

struct TitleIconStateData
	{
// These two rects are used as normal
	Rect fUserState;
	Rect fStdState;
// Offset of the left edge of the icon
	short fLeftEdge;
// if non zero then draw text and icon in black (or approp color) even if the window is not hilited
	short fHiliteTitleAnyway;
// The icon's mono and mask pixels (16 x 16, same as an SICN)
	char fBits[64];
	};

static Handle sTitleIconResource = nil;

static bool sHasTitleIconResource()
	{
	static bool checked = false;
	if (!checked)
		{
		sTitleIconResource = ::Get1Resource('WDEF', 6);
		checked = true;
		}
	return sTitleIconResource != nil;
	}

static bool sIsTitleIconWindow(WindowRef inWindowRef)
	{
	if (!sHasTitleIconResource())
		return false;
	WindowPeek_ theWindowPeek = (WindowPeek_)inWindowRef;
	Handle wDefProcHandle = theWindowPeek->windowDefProc;
	return wDefProcHandle == sTitleIconResource;
	}

static TitleIconStateData** sGetTitleIconStateData(WindowRef inWindowRef)
	{
	WindowPeek_ theWindowPeek = (WindowPeek_)inWindowRef;
	if (::GetHandleSize(theWindowPeek->dataHandle) != sizeof(TitleIconStateData))
		return nil;
	return (TitleIconStateData**)theWindowPeek->dataHandle;
	}

static void sSetTitleIcon(WindowRef inWindowRef, const ZDCPixmap& inPixmap, const ZDCPixmap& inMaskPixmap)
	{
	TitleIconStateData** theStateData = ::sGetTitleIconStateData(inWindowRef);
	if (!theStateData)
		return;

	ZUtil_Mac_LL::HandleLocker stateDataLocker((Handle)theStateData);

	ZDCPixmapNS::RasterDesc theRasterDesc;
	theRasterDesc.fPixvalDesc.fDepth = 1;
	theRasterDesc.fPixvalDesc.fBigEndian = true;
	theRasterDesc.fRowBytes = 2;
	theRasterDesc.fRowCount = 16;
	theRasterDesc.fFlipped = false;
	ZDCPixmapNS::PixelDesc thePixelDesc(1, 0);
	inPixmap.CopyTo(ZPoint(0, 0), &theStateData[0]->fBits[0], theRasterDesc, thePixelDesc, ZRect(16, 16));
//##	inMaskPixmap.CopyTo(ZPoint(0, 0), &theStateData[0]->fBits[32], theRasterDesc, thePixelDesc, ZRect(16, 16), true);
	inMaskPixmap.CopyTo(ZPoint(0, 0), &theStateData[0]->fBits[32], theRasterDesc, thePixelDesc, ZRect(16, 16));

	stateDataLocker.Orphan();

// Force the window to redraw by setting its title
	Str255 tempTitle;
	::GetWTitle(inWindowRef, tempTitle);
	::SetWTitle(inWindowRef, tempTitle);
	}

static void sGetContentInsets(WindowRef inWindowRef, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
// This concept was ripped off from MacApp. We want to find out the difference between
// the structure boundary and the content boundary. Just because a window has been
// created, doesn't mean that the window regions have been generated. If the window's
// never been shown then the regions will be empty, or at least invalid. So, we do something
// skanky and call the window's definition procedure directly, telling it to calculate
// the regions.

	ZDCRgn structureRgn;
	ZDCRgn contentRgn;
	WindowPeek_ theWindowPeek = (WindowPeek_)inWindowRef;

	if (!MacIsWindowVisible(inWindowRef) && ::EmptyRgn(theWindowPeek->strucRgn))
		{
// If the window is not visible and its structure region is empty, then we need to
// call the window definition procedure.
		Handle wDefProcHandle = theWindowPeek->windowDefProc;
		ZAssertStop(kDebug_Mac, wDefProcHandle);
		if (!*wDefProcHandle)
			::LoadResource(wDefProcHandle);
		SignedByte savedState = ::HGetState(wDefProcHandle);
		::HLock(wDefProcHandle);
		WindowDefProcPtr wDefProc = *(WindowDefProcPtr*)wDefProcHandle;
		CallWindowDefProc((WindowDefUPP)wDefProc, ::GetWVariant(inWindowRef), inWindowRef, wCalcRgns, 0L);
		::HSetState(wDefProcHandle, savedState);
		structureRgn = theWindowPeek->strucRgn;
		contentRgn = theWindowPeek->contRgn;
// Reset the window regions as empty, otherwise window manager may get confused
		::SetEmptyRgn(theWindowPeek->strucRgn);
		::SetEmptyRgn(theWindowPeek->contRgn);
		}
	else
		{
		structureRgn = theWindowPeek->strucRgn;
		contentRgn = theWindowPeek->contRgn;
		}

	outTopLeftInset = contentRgn.Bounds().TopLeft() - structureRgn.Bounds().TopLeft();
	outBottomRightInset = contentRgn.Bounds().BottomRight() - structureRgn.Bounds().BottomRight();
	}

static ZRect sGetFrame(WindowRef inWindowRef)
	{
// Window global bounds is local to global of the portRect.
	ZUtil_Mac_LL::SaveRestorePort theSRP;
	CGrafPtr windowPort = GetWindowPort(inWindowRef);
	::SetGWorld(windowPort, nil);
	Rect theFrame = windowPort->portRect;
	::LocalToGlobal(reinterpret_cast<Point*>(&theFrame.top));
	::LocalToGlobal(reinterpret_cast<Point*>(&theFrame.bottom));
	return theFrame;
	}

static ZPoint sGetSize(WindowRef inWindowRef)
	{
	CGrafPtr windowPort = GetWindowPort(inWindowRef);
	return ZPoint(windowPort->portRect.right - windowPort->portRect.left, windowPort->portRect.bottom - windowPort->portRect.top);
	}

static ZPoint sGetLocation(WindowRef inWindowRef)
	{
	ZUtil_Mac_LL::SaveRestorePort theSRP;
	CGrafPtr windowPort = GetWindowPort(inWindowRef);
	::SetGWorld(windowPort, nil);
	Point theTopLeft = *reinterpret_cast<Point*>(&windowPort->portRect);
	::LocalToGlobal(&theTopLeft);
	return theTopLeft;
	}

static ZPoint sFromGlobal(WindowRef inWindowRef, ZPoint inGlobalPoint)
	{
	ZUtil_Mac_LL::SaveRestorePort theSRP;
	CGrafPtr windowPort = GetWindowPort(inWindowRef);
	::SetGWorld(windowPort, nil);
	Point localPoint = inGlobalPoint;
	::GlobalToLocal(&localPoint);
// Compensate for whatever the window's port origin currently is
	localPoint.h -= windowPort->portRect.left;
	localPoint.v -= windowPort->portRect.top;
	return localPoint;
	}

static void sSendBehind(WindowRef inWindowToMove, WindowRef inOtherWindow)
	{
// This won't do anything if windowToMove is already behind otherWindow
	if (inOtherWindow == (WindowRef)-1L)
		{
		if (::GetWindowList() != inWindowToMove)
			::BringToFront(inWindowToMove);
		}
	else if (inOtherWindow == nil)
		::SendBehind(inWindowToMove, nil);
	else
		{
// You might think this first check is dumb -- but it ain't!
		if (inOtherWindow != inWindowToMove && MacGetNextWindow(inOtherWindow) != inWindowToMove)
			::SendBehind(inWindowToMove, inOtherWindow);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac::DragInfo

class ZOSWindow_Mac::DragInfo
	{
public:
	ZDragInitiator* fDragInitiator;
	ZDragSource* fDragSource;
	const ZTuple* fTuple;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac::DragInitiator

class ZOSWindow_Mac::DragInitiator : public ZDragInitiator
	{
public:
	DragInitiator();
	~DragInitiator();

// From ZDragInitiator
	virtual void Imp_DoDrag(const ZTuple& inTuple, ZPoint inStartPoint, ZPoint inHotPoint,
							const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap,
							const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap, ZDragSource* inDragSource);

// Our protocol
	void DragThread();
	static void sDragThread(DragInitiator* inDragInitiator);

protected:
	ZSemaphore fSemaphore;

	ZTuple fTuple;

	ZPoint fStartPoint;
	ZPoint fHotPoint;

	ZDCRgn* fRgn;
	ZRef<ZDCPixmapRep_QD> fPixmapRep;
	ZDCPixmap* fPixmap;
	ZDCRgn* fMaskRgn;
	ZDCPixmap* fMaskPixmap;

	ZDragSource* fDragSource;
	};

ZOSWindow_Mac::DragInitiator::DragInitiator()
:	fSemaphore(0, "ZOSWindow_Mac::DragInitiator::fSemaphore")
	{
	fRgn = nil;
	fPixmap = nil;
	fMaskRgn = nil;
	fMaskPixmap = nil;
	fDragSource = nil;

// Create the thread, which will block on fSemaphore and will only proceed once Imp_DoDrag is called.
// We could create the thread in Imp_DoDrag, but this way we'll throw an exception if the thread could not
// be created and a nil DragInitiator will be returned, so our ultimate caller can take remedial action (ie give up).
	(new ZThreadSimple<DragInitiator*>(sDragThread, this, "ZOSWindow_Mac::DragInitiator::sDragThread"))->Start();
	}

ZOSWindow_Mac::DragInitiator::~DragInitiator()
	{
	delete fRgn;
	delete fPixmap;
	delete fMaskRgn;
	delete fMaskPixmap;
	}

// kDragInfoOSType is an arbitrary tag -- we just need to be sure that it's not used by any other
// app that might be dragging data *into* our process. When the DragInfo data is retrieved by ZDrop,
// it makes sure that the size of the data matches the sizeof DragInfo as an additional safety check.
static const unsigned long kDragInfoOSType = 'ZLIB';

void ZOSWindow_Mac::DragInitiator::Imp_DoDrag(const ZTuple& inTuple, ZPoint inStartPoint, ZPoint inHotPoint,
							const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap,
							const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap, ZDragSource* inDragSource)
	{
// We need either a drag rgn, or a drag pixmap (or both)
	ZAssertStop(kDebug_Mac, inDragRgn || inDragPixmap);

	fTuple = inTuple;
	fStartPoint = inStartPoint;
	fHotPoint = inHotPoint;

	if (inDragRgn)
		fRgn = new ZDCRgn(*inDragRgn);

	if (inDragPixmap)
		fPixmapRep = ZDCPixmapRep_QD::sAsPixmapRep_QD(inDragPixmap->GetRep());

	if (inDragMaskRgn)
		fMaskRgn = new ZDCRgn(*inDragMaskRgn);

	if (inDragMaskPixmap)
		fMaskPixmap = new ZDCPixmap(*inDragMaskPixmap);

	fDragSource = inDragSource;
// We have all the info, so release our semaphore
	fSemaphore.Signal(1);
	}

void ZOSWindow_Mac::DragInitiator::DragThread()
	{
// Wait till Imp_DoDrag is called
	fSemaphore.Wait(1);
// The real work

	DragReference theDragRef;
	OSErr err = ::NewDrag(&theDragRef);

	ItemReference currentItemReference = 1;

// Set up theDragInfo struct with useful information a drop can make use of, and add a pointer to it to the first (and only) drag item
	ZOSWindow_Mac::DragInfo theDragInfo;
	theDragInfo.fDragInitiator = this;
	theDragInfo.fDragSource = fDragSource;
	theDragInfo.fTuple = &fTuple;
	ZOSWindow_Mac::DragInfo* tempDragInfoPtr = &theDragInfo;
	::AddDragItemFlavor(theDragRef, currentItemReference, kDragInfoOSType, &tempDragInfoPtr, sizeof(tempDragInfoPtr), flavorSenderOnly | flavorNotSaved);

// Walk through inTuple's data and add to the drag those items that will make it outside our process
	for (ZTuple::const_iterator iter = fTuple.begin(); iter != fTuple.end(); ++iter)
		{
		const string propertyName = fTuple.NameOf(iter);
		OSType theOSType;
		bool isString;
		if (ZDragClipManager::sGet()->LookupMIME(propertyName, theOSType, isString))
			{
			ZType propertyType = fTuple.TypeOf(iter);
			if (isString && propertyType == eZType_String)
				{
				string theString = fTuple.GetString(iter);
				if (theString.size())
					::AddDragItemFlavor(theDragRef, currentItemReference, theOSType, theString.data(), theString.size(), 0);
				else
					::AddDragItemFlavor(theDragRef, currentItemReference, theOSType, nil, 0, 0);
				}
			else if (!isString && propertyType == eZType_Raw)
				{
				size_t rawSize;
				const void* rawAddress;
				bool okay = fTuple.GetRawAttributes(iter, &rawAddress, &rawSize);
				ZAssertStop(1, okay);
				::AddDragItemFlavor(theDragRef, currentItemReference, theOSType, rawAddress, rawSize, 0);
				}
			}		
		}

	ZDCRgn dragImageRgn; // These both have to live longer than the drag that might use it, hence they're declared here
	PixMapHandle dragPixMapHandle = nil;
	if (fPixmap && *fPixmap)
		{
		if (fMaskPixmap && *fMaskPixmap)
			{
			dragImageRgn = ZRect(fMaskPixmap->Size());
//			dragImageRgn = fMaskPixmap->AsRgn(ZRGBColor::sWhite, true);
			}
		else if (fMaskRgn)
			{
			dragImageRgn = *fMaskRgn;
			}

// Check for regions which are larger than the pixmap -- this causes graphical glitches, so we trip an assertion here
#if ZCONFIG_Debug
		if (dragImageRgn)
			{
			ZPoint pixmapSize = fPixmap->Size();
			ZRect dragImageRgnBounds = dragImageRgn.Bounds();
			if (dragImageRgnBounds.top < 0 || dragImageRgnBounds.left < 0 || dragImageRgnBounds.right > pixmapSize.h || dragImageRgnBounds.bottom > pixmapSize.v)
				ZDebugStopf(1, ("This region will cause crap to get drawn on screen"));
			}
#endif // ZCONFIG_Debug
		DragImageFlags theFlags = kDragDarkTranslucency; // kDragStandardTranslucency;
		if (fRgn)
			theFlags |= kDragRegionAndImage;

// Take a straight copy of the PixMap data from our ZDCPixmap
		dragPixMapHandle = PixMapHandle(::NewHandle(sizeof(PixMap)));
		ZUtil_Mac_LL::HandleLocker locker(Handle(dragPixMapHandle));
		dragPixMapHandle[0][0] = fPixmapRep->GetPixMap();

		err = ::SetDragImage(theDragRef, dragPixMapHandle, dragImageRgn ? dragImageRgn.GetRgnHandle() : (RgnHandle)nil, fStartPoint - fHotPoint, theFlags);
		}

	EventRecord theEventRecord;
	theEventRecord.what = mouseDown;
	theEventRecord.message = 0;
	theEventRecord.when = ::TickCount();
	theEventRecord.where = fStartPoint;
	theEventRecord.modifiers = 0;

// We need to make sure our current port is not offscreen (took me two hours to figure that out). So lets set the
// current port to be the window manager port. We don't restore it afterwards because threading issues could
// cause the port that exists and is current right now to no longer exist when we'er done.
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
	ZUtil_Mac_LL::sSetWindowManagerPort();

// AG 99-01-09. I just spent all evening figuring out why we crash out really badly in all systems prior to 8.5. The solution
// is that we *must* pass a RgnHandle to drag -- nil is not okay.
	ZDCRgn localRgn;
	if (fRgn)
		localRgn = *fRgn + (fStartPoint - fHotPoint);
	else
		{	
		if (fMaskPixmap || fMaskRgn)
			localRgn = dragImageRgn;
		else
			localRgn = ZRect(fPixmap->Size());
		localRgn += (fStartPoint - fHotPoint);
		localRgn.MakeOutline();
		}
	err = ::TrackDrag(theDragRef, &theEventRecord, localRgn.GetRgnHandle());

	::DisposeDrag(theDragRef);

	if (dragPixMapHandle)
		::DisposeHandle(Handle(dragPixMapHandle));

// Our caller handed lifetime responsibility to us. On Windows we use reference counting because of subtle
// interactions with OLE/COM. Here we just delete ourselves.
	delete this;
	}

void ZOSWindow_Mac::DragInitiator::sDragThread(DragInitiator* inDragInitiator)
	{ inDragInitiator->DragThread(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac::DragDropImpl

class ZOSWindow_Mac::DragDropImpl
	{
public:
	DragDropImpl(DragReference inDragReference);
	~DragDropImpl();

	size_t Internal_CountTuples() const;
	ZTuple Internal_GetTuple(size_t inItemNumber) const;

protected:
	const ZTuple* fTuple;
	ZDragSource* fDragSource;

	DragReference fDragReference;
	};

ZOSWindow_Mac::DragDropImpl::DragDropImpl(DragReference inDragReference)
:	fTuple(nil), fDragSource(nil), fDragReference(inDragReference)
	{
// See if this is an intra-process drag, in which case we can mostly bypass the drag manager's API
	unsigned short numItems;
	if (noErr != ::CountDragItems(fDragReference, &numItems))
		return;
	if (numItems == 1)
		{
		ItemReference theItemReference;
		if (noErr != ::GetDragItemReferenceNumber(fDragReference, 1, &theItemReference))
			return;

		Size theSize;
		if (noErr != ::GetFlavorDataSize(fDragReference, theItemReference, kDragInfoOSType, &theSize))
			return;
		if (theSize != sizeof(ZOSWindow_Mac::DragInfo*))
			return;
		ZOSWindow_Mac::DragInfo* theDragInfo;
		if (noErr != ::GetFlavorData(fDragReference, theItemReference, kDragInfoOSType, &theDragInfo, &theSize, 0))
			return;
		if (theDragInfo)
			{
			fTuple = theDragInfo->fTuple;
			fDragSource = theDragInfo->fDragSource;
			}
		}
	}

ZOSWindow_Mac::DragDropImpl::~DragDropImpl()
	{}

size_t ZOSWindow_Mac::DragDropImpl::Internal_CountTuples() const
	{
	if (fTuple)
		return 1;
	unsigned short numItems;
	OSErr err = ::CountDragItems(fDragReference, &numItems);
	if (err == noErr)
		return numItems;
	return 0;
	}

ZTuple ZOSWindow_Mac::DragDropImpl::Internal_GetTuple(size_t inItemNumber) const
	{
	ZTuple theTuple;
	if (fTuple)
		{
		ZAssertStop(kDebug_Mac, inItemNumber == 0);
		theTuple = *fTuple;
		}
	else
		{
		UInt16 numItems;
		if (noErr == ::CountDragItems(fDragReference, &numItems))
			{
			ZAssertStop(kDebug_Mac, inItemNumber < numItems);

			ItemReference theItemReference;
			if (noErr == ::GetDragItemReferenceNumber(fDragReference, inItemNumber + 1, &theItemReference))
				{
				UInt16 flavorCount;
				if (noErr == ::CountDragItemFlavors(fDragReference, theItemReference, &flavorCount))
					{
					for (size_t currentFlavorNumber = 1; currentFlavorNumber <= flavorCount; ++currentFlavorNumber)
						{
						FlavorType theFlavorType;
						if (noErr == ::GetFlavorType(fDragReference, theItemReference, currentFlavorNumber, &theFlavorType))
							{
							bool isString;
							string thePropertyName;
							if (ZDragClipManager::sGet()->LookupScrapFlavorType(theFlavorType, thePropertyName, isString))
								{
								Size theSize;
								if (noErr == ::GetFlavorDataSize(fDragReference, theItemReference, theFlavorType, &theSize))
									{
									try
										{
										if (isString)
											{
											string theString((size_t)theSize, ' ');
											if (theSize == 0 || noErr == ::GetFlavorData(fDragReference, theItemReference, theFlavorType, &theString[0], &theSize, 0))
												theTuple.SetString(thePropertyName, theString);
											}
										else
											{
											if (theSize > 0)
												{
												vector<char> buffer(theSize);
												if (noErr == ::GetFlavorData(fDragReference, theItemReference, theFlavorType, &buffer[0], &theSize, 0))
													theTuple.SetRaw(thePropertyName, &buffer[0], theSize);
												}
											else
												{
												theTuple.SetRaw(thePropertyName, nil, 0);
												}
											}
										}
									catch (...)
										{}
									}
								}
							}
						}
					}
				}
			}
		}
	return theTuple;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac::Drag

class ZOSWindow_Mac::Drag : public ZDrag, protected ZOSWindow_Mac::DragDropImpl
	{
public:
	Drag(DragReference inDragReference);
	~Drag();

// From ZDrag
	virtual size_t CountTuples() const;
	virtual ZTuple GetTuple(size_t inItemNumber) const;
	virtual ZDragSource* GetDragSource() const;
	virtual void GetNative(void* outNative);
	};

ZOSWindow_Mac::Drag::Drag(DragReference inDragReference)
:	DragDropImpl(inDragReference)
	{}

ZOSWindow_Mac::Drag::~Drag()
	{}

// From ZDrag
size_t ZOSWindow_Mac::Drag::CountTuples() const
	{ return this->Internal_CountTuples(); }

ZTuple ZOSWindow_Mac::Drag::GetTuple(size_t inItemNumber) const
	{ return this->Internal_GetTuple(inItemNumber); }

ZDragSource* ZOSWindow_Mac::Drag::GetDragSource() const
	{ return fDragSource; }

void ZOSWindow_Mac::Drag::GetNative(void* outNative)
	{ *reinterpret_cast<DragReference*>(outNative) = fDragReference; }

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac::Drop

class ZOSWindow_Mac::Drop : public ZDrop, protected ZOSWindow_Mac::DragDropImpl
	{
public:
	Drop(DragReference inDragReference);
	~Drop();

// From ZDrop
	virtual size_t CountTuples() const;
	virtual ZTuple GetTuple(size_t inItemNumber) const;
	virtual ZDragSource* GetDragSource() const;
	virtual void GetNative(void* outNative);
	};

ZOSWindow_Mac::Drop::Drop(DragReference inDragReference)
:	DragDropImpl(inDragReference)
	{}

ZOSWindow_Mac::Drop::~Drop()
	{}

// From ZDrag
size_t ZOSWindow_Mac::Drop::CountTuples() const
	{ return this->Internal_CountTuples(); }

ZTuple ZOSWindow_Mac::Drop::GetTuple(size_t inItemNumber) const
	{ return this->Internal_GetTuple(inItemNumber); }

ZDragSource* ZOSWindow_Mac::Drop::GetDragSource() const
	{ return fDragSource; }

void ZOSWindow_Mac::Drop::GetNative(void* outNative)
	{ *reinterpret_cast<DragReference*>(outNative) = fDragReference; }

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_Window

class ZDCCanvas_QD_Window : public ZDCCanvas_QD
	{
public:
	ZDCCanvas_QD_Window(ZOSWindow_Mac* inWindow, CGrafPtr inGrafPtr, ZMutex* inMutexToCheck, ZMutex* inMutexToLock);
	virtual ~ZDCCanvas_QD_Window();

// Our protocol
	void OSWindowDisposed();

// From ZRefCountedWithFinalization via ZDCCanvas_QD
	virtual void Finalize();

// From ZDCCanvas via ZDCCanvas_QD
	virtual void Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord inDeltaH, ZCoord inDeltaV);
	virtual void CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect);

	virtual ZRef<ZDCCanvas> CreateOffScreen(const ZRect& inCanvasRect);

// From ZDCCanvas_QD
	virtual ZDCRgn Internal_CalcClipRgn(const ZDCState& inState);
	virtual ZDCRgn Internal_GetExcludeRgn();

// Our protocol
	void InvalClip();

protected:
	ZOSWindow_Mac* fOSWindow;
	};

ZDCCanvas_QD_Window::ZDCCanvas_QD_Window(ZOSWindow_Mac* inWindow, CGrafPtr inGrafPtr, ZMutex* inMutexToCheck, ZMutex* inMutexToLock)
	{
	ZMutexLocker locker(sMutex_List);
	fOSWindow = inWindow;
	fMutexToCheck = inMutexToCheck;
	fMutexToLock = inMutexToLock;
	this->Internal_Link(inGrafPtr);
	}

ZDCCanvas_QD_Window::~ZDCCanvas_QD_Window()
	{
	ZAssertLocked(kDebug_Mac, sMutex_List);
	fGrafPtr = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	this->Internal_Unlink();
	}

// Our protocol
void ZDCCanvas_QD_Window::OSWindowDisposed()
	{
	fOSWindow = nil;
	fGrafPtr = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	}

// From ZRefCountedWithFinalization via ZDCCanvas_QD
void ZDCCanvas_QD_Window::Finalize()
	{
	ZMutexLocker locker(sMutex_List);

	if (fOSWindow)
		{
		fOSWindow->FinalizeCanvas(this);
		}
	else
		{
// If we don't have a window, fMutexToCheck and fMutexToLock must already be nil
		ZAssertStop(kDebug_Mac, fMutexToCheck == nil && fMutexToLock == nil);
		this->FinalizationComplete();
		delete this;
		}
	}

void ZDCCanvas_QD_Window::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fGrafPtr)
		return;

	ZAssertLocked(kDebug_Mac, *fMutexToCheck);

	ZMutexLocker locker(*fMutexToLock);

	// Mark our clip as having been changed, because it's going to be physically changed to do the scroll
	// or we're going to call Internal_RecordInvalidation, which will also invalidate our clip.
	++fChangeCount_Clip;

	// We don't bother ensuring that our grafPort's origin is at (0,0), we just compensate for whatever
	// value it has by adding in currentQDOrigin as needed.
	ZPoint currentQDOrigin(fGrafPtr->portRect.left, fGrafPtr->portRect.top);
	ZPoint offset(hDelta, vDelta);

	// Work out effectiveVisRgn, the set of pixels we're able/allowed to touch. It's made
	// up of our port's real visRgn, intersected with inRect, and with our window's exclude rgn removed
	ZDCRgn effectiveVisRgn;
	effectiveVisRgn = fGrafPtr->visRgn;
	// As visRgn is kept in port coords we have shift it to our window coordinate system.
	effectiveVisRgn -= currentQDOrigin;
	effectiveVisRgn &= inRect + ioState.fOrigin;
	effectiveVisRgn -= fOSWindow->Internal_GetExcludeRgn();

	// destRgn is the pixels we want and are able to draw to.
	ZDCRgn destRgn = (ioState.fClip + ioState.fClipOrigin) & effectiveVisRgn;

	// srcRgn is the set of pixels we're want and are able to copy from.
	ZDCRgn srcRgn = (destRgn - offset) & effectiveVisRgn;

	// drawnRgn is the destination pixels that will be drawn by the CopyBits call, it's the srcRgn
	// shifted back to the destination.
	ZDCRgn drawnRgn = srcRgn + offset;

	// invalidRgn is the destination pixels we wanted to draw but could not because they were
	// outside the visRgn, or were in the excludeRgn
	ZDCRgn invalidRgn = destRgn - drawnRgn;

	// Now do the jiggery-pokery to actually scroll:
#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// And set the clip (drawnRgn)
	::SetClip((drawnRgn + currentQDOrigin).GetRgnHandle());

	Rect qdSourceRect = drawnRgn.Bounds() + currentQDOrigin - offset;
	Rect qdDestRect = drawnRgn.Bounds() + currentQDOrigin;
	::CopyBits(GetPortBitMapForCopyBits(fGrafPtr), GetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);

	// Finally, let our window know what additional pixels are invalid
	fOSWindow->Internal_RecordInvalidation(invalidRgn);
	}

void ZDCCanvas_QD_Window::CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
	ZRef<ZDCCanvas_QD> sourceCanvasQD = ZRefDynamicCast<ZDCCanvas_QD>(inSourceCanvas);
	ZAssertStop(kDebug_Mac, sourceCanvasQD);

	GrafPtr sourceGrafPtr = (GrafPtr)sourceCanvasQD->Internal_GetGrafPtr();
	if (fGrafPtr == nil || sourceGrafPtr == nil)
		return;

	ZAssertLocked(kDebug_Mac, *fMutexToCheck);

	ZMutexLocker locker(*fMutexToLock);

	++fChangeCount_Clip;

	ZRect localDestRect = inSourceRect + (ioState.fOrigin + inDestLocation - inSourceRect.TopLeft());
	ZPoint currentDestQDOrigin(fGrafPtr->portRect.left, fGrafPtr->portRect.top);

	ZDCRgn effectiveDestVisRgn;
	effectiveDestVisRgn = fGrafPtr->visRgn;
	effectiveDestVisRgn -= currentDestQDOrigin;
	effectiveDestVisRgn &= localDestRect;
	effectiveDestVisRgn -= fOSWindow->Internal_GetExcludeRgn();

	ZDCRgn destRgn = (ioState.fClip + ioState.fClipOrigin) & effectiveDestVisRgn;

	ZRect localSourceRect = inSourceRect + inSourceState.fOrigin;
	ZPoint currentSourceQDOrigin(sourceGrafPtr->portRect.left, sourceGrafPtr->portRect.top);

	ZDCRgn effectiveSourceVisRgnTranslatedToDestCoords;
	effectiveSourceVisRgnTranslatedToDestCoords = sourceGrafPtr->visRgn;
	effectiveSourceVisRgnTranslatedToDestCoords -= currentSourceQDOrigin;
	effectiveSourceVisRgnTranslatedToDestCoords &= localSourceRect;
	effectiveSourceVisRgnTranslatedToDestCoords -= sourceCanvasQD->Internal_GetExcludeRgn();
	effectiveSourceVisRgnTranslatedToDestCoords += localDestRect.TopLeft() - localSourceRect.TopLeft();

	ZDCRgn drawnRgn = destRgn & effectiveSourceVisRgnTranslatedToDestCoords;

	ZDCRgn invalidRgn = destRgn - drawnRgn;

#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

// And set the clip (drawnRgn)
	::SetClip((drawnRgn + currentDestQDOrigin).GetRgnHandle());

	Rect qdSourceRect = drawnRgn.Bounds() + (currentSourceQDOrigin + localSourceRect.TopLeft() - localDestRect.TopLeft());
	Rect qdDestRect = drawnRgn.Bounds() + currentDestQDOrigin;
	::CopyBits(GetPortBitMapForCopyBits(sourceGrafPtr), GetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);

// Finally, let our window know what additional pixels are invalid
	fOSWindow->Internal_RecordInvalidation(invalidRgn);
	}

ZRef<ZDCCanvas> ZDCCanvas_QD_Window::CreateOffScreen(const ZRect& inCanvasRect)
	{
	if (!fGrafPtr)
		return ZRef<ZDCCanvas>();

	ZAssertLocked(kDebug_Mac, *fMutexToCheck);

#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	::SetGWorld(fGrafPtr, nil);

	Point zeroPoint;
	zeroPoint.h = fGrafPtr->portRect.left;
	zeroPoint.v = fGrafPtr->portRect.top;
	::LocalToGlobal(&zeroPoint);

	return new ZDCCanvas_QD_OffScreen(inCanvasRect + zeroPoint);
	}

ZDCRgn ZDCCanvas_QD_Window::Internal_CalcClipRgn(const ZDCState& inState)
	{
	return (inState.fClip + inState.fClipOrigin) - fOSWindow->Internal_GetExcludeRgn();
	}

ZDCRgn ZDCCanvas_QD_Window::Internal_GetExcludeRgn()
	{ return fOSWindow->Internal_GetExcludeRgn(); }

void ZDCCanvas_QD_Window::InvalClip()
	{
	ZAssertStop(kDebug_Mac, fMutexToLock->IsLocked());
	++fChangeCount_Clip;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac

ZOSWindow_Mac::ZOSWindow_Mac(WindowRef inWindowRef)
:	fCursor(ZCursor::eCursorTypeArrow)
	{
	ZAssertStop(kDebug_Mac, inWindowRef);
	fCanvas = nil;

	fWindowActive = false;
	fWindowRef = inWindowRef;
	::SetWRefCon(fWindowRef, (long)this);

	fSize_Min.h = 20;
	fSize_Min.v = 20;
	fSize_Max.h = 20000;
	fSize_Max.v = 20000;

	ZAssertStop(kDebug_Mac, (GetWindowKind(fWindowRef) & kKindBits) == kZOSWindow_MacKind);

	this->Internal_RecordInvalidation(sGetFrame(fWindowRef));
	this->Internal_RecordFrameChange(sGetFrame(fWindowRef));
	this->Internal_ReportFrameChange();

	ZOSApp_Mac::sGet()->AddOSWindow(this);
	}

ZOSWindow_Mac::~ZOSWindow_Mac()
	{
	ASSERTLOCKED();

	ZOSApp_Mac::sGet()->ShowWindow(this, false);

	if (ZRef<ZDCCanvas_QD_Window> tempCanvas = fCanvas)
		{
		fCanvas = nil;
		tempCanvas->OSWindowDisposed();
		}

	ZOSApp_Mac::sGet()->RemoveOSWindow(this);
	::SetWRefCon(fWindowRef, 0);

#if ZCONFIG(Processor, PPC)
// I'm not sure if it's necessary to do this remove of the proxy, but it can't hurt.
	if (::sGestalt_WindowManager() && ::sGestalt_IconServices())
		::RemoveWindowProxy(fWindowRef);
#endif // ZCONFIG(Processor, PPC)

// If the current port is this window, reset it to the window manager port (for want of anywhere better)
	CGrafPtr currentPort;
	GDHandle currentGDHandle;
	::GetGWorld(&currentPort, &currentGDHandle);
	if (currentPort == GetWindowPort(fWindowRef))
		ZUtil_Mac_LL::sSetWindowManagerPort();

	WindowRef myWindowRef = fWindowRef;
	fWindowRef= nil;
	::DisposeWindow(myWindowRef);
	}

bool ZOSWindow_Mac::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow_Std::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:MacMenuClick")
			{
			if (fOwner)
				{
				::InitCursor();
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				if (ZRef<ZMenuItem> theMenuItem = ZOSApp_Mac::sGet()->PostSetupAndHandleMenuBarClick(fMenuBar, inMessage.GetPoint("where")))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		else if (theWhat == "zoolib:MacMenuKey")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				EventRecord theEventRecord;
				inMessage.GetRaw("eventRecord", &theEventRecord, sizeof(EventRecord));
				if (ZRef<ZMenuItem> theMenuItem = ZOSApp_Mac::sGet()->PostSetupAndHandleCammandKey(fMenuBar, theEventRecord))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		}
	return false;
	}

void ZOSWindow_Mac::SetShownState(ZShownState inState)
	{
	ASSERTLOCKED();
	ZOSApp_Mac::sGet()->ShowWindow(this, inState != eZShownStateHidden);
	this->Internal_ReportShownStateChange();
	this->Internal_ReportActiveChange();
	}

void ZOSWindow_Mac::SetTitle(const string& inTitle)
	{
	ASSERTLOCKED();
	Str255 oldTitle;
	::GetWTitle(fWindowRef, oldTitle);
	Str255 newTitle;
	ZString::sToPString(inTitle, newTitle, 255);
	if (oldTitle[0] != newTitle[0] || memcmp(oldTitle, newTitle, oldTitle[0]))
		::SetWTitle(fWindowRef, newTitle);
	}

string ZOSWindow_Mac::GetTitle()
	{
	ASSERTLOCKED();
	Str255 tempTitle;
	::GetWTitle(fWindowRef, tempTitle);
	return ZString::sFromPString(tempTitle);
	}

void ZOSWindow_Mac::SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap)
	{
	ASSERTLOCKED();
	if (false) {}
// First try to use TitleIconBar
	else if (::sIsTitleIconWindow(fWindowRef))
		{
		::sSetTitleIcon(fWindowRef, inMonoPixmap ? inMonoPixmap : inColorPixmap, inMaskPixmap);
		return;
		}
#if ZCONFIG(Processor, PPC)
	else if (::sGestalt_WindowManager() && ::sGestalt_IconServices())
		{
		if ((inColorPixmap || inMonoPixmap) && inMaskPixmap)
			{
			if (IconRef theIconRef = ZUtil_Mac_HL::sIconRefFromPixmaps(inColorPixmap, inMonoPixmap, inMaskPixmap))
				{
				::SetWindowModified(fWindowRef, false);
				::SetWindowProxyIcon(fWindowRef, theIconRef);
				::ReleaseIconRef(theIconRef);
				}
			}
		else
			{
			::RemoveWindowProxy(fWindowRef);
			}
		}
#endif // ZCONFIG(Processor, PPC)
	}

ZCoord ZOSWindow_Mac::GetTitleIconPreferredHeight()
	{
	ASSERTLOCKED();
	return 16;
	}

void ZOSWindow_Mac::SetCursor(const ZCursor& inCursor)
	{
	ASSERTLOCKED();
	fCursor = inCursor;
	ZOSApp_Mac::sGet()->CursorChanged(this);
	}

void ZOSWindow_Mac::SetLocation(ZPoint inLocation)
	{
	ASSERTLOCKED();
	this->Internal_SetLocation(inLocation);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Mac::SetSize(ZPoint inSize)
	{
	ASSERTLOCKED();
	this->Internal_SetSize(inSize);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Mac::SetFrame(const ZRect& inFrame)
	{
	ASSERTLOCKED();
// This routine ensures a window's frame matches inFrame. If inFrame is such that the only the location or
// only the size is changing, then it decays to a call to SetLocation or SetSize appropriately. Otherwise
// it will try and use a call to zoom the window, which will change both simultaneously, but may
// cause an invalidation of the entire window contents on older versions of MacOS. If the window cannot
// be zoomed, then it sizes and locates the window in two steps.
	ZRect localRect = inFrame;
	localRect.right = min(localRect.right, ZCoord(localRect.left + fSize_Max.h));
	localRect.bottom = min(localRect.bottom, ZCoord(localRect.top + fSize_Max.v));

	localRect.right = max(localRect.right, ZCoord(localRect.left + fSize_Min.h));
	localRect.bottom = max(localRect.bottom, ZCoord(localRect.top + fSize_Min.v));

	ZRect oldFrame = ::sGetFrame(fWindowRef);
	SetPortWindowPort(fWindowRef);
	if (localRect.Size() == oldFrame.Size())
		{
		::MacMoveWindow(fWindowRef, localRect.left, localRect.top, false);
		this->Internal_RecordLocationChange(localRect.TopLeft());
		}
	else if (localRect.TopLeft() == oldFrame.TopLeft())
		{
		::SizeWindow(fWindowRef, localRect.Width(), localRect.Height(), true);
		this->Internal_RecordSizeChange(localRect.Size());
		}
	else
		{
		if (GetWindowZoomFlag(fWindowRef))
			{
			Rect oldRect;
			GetWindowStandardState(fWindowRef, &oldRect);
			Rect tempRect(localRect);
			SetWindowStandardState(fWindowRef, &tempRect);
			ZUtil_Mac_LL::SaveRestorePort theSRP;
//			SetPortWindowPort(fWindowRef);
			::ZoomWindow(fWindowRef, inZoomOut, false);
			SetWindowStandardState(fWindowRef, &oldRect);
			}
		else
			{
			::MacMoveWindow(fWindowRef, localRect.left, localRect.top, false);
			::SizeWindow(fWindowRef, localRect.Width(), localRect.Height(), true);
			}
		this->Internal_RecordFrameChange(localRect);
		}
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_Mac::SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize)
	{
	ASSERTLOCKED();
// Force into reasonable ranges
	if (inMinSize.h <= 0)
		inMinSize.h = 1;
	if (inMinSize.v <= 0)
		inMinSize.v = 1;
	if (inMaxSize.h < inMinSize.h)
		inMaxSize.h = inMinSize.h;
	if (inMaxSize.v < inMinSize.v)
		inMaxSize.v = inMinSize.v;

// Record the limits
	fSize_Min = inMinSize;
	fSize_Max = inMaxSize;
// Tell ourselves to resize to our current size. If our current size falls outside of the
// acceptable range, then it will get coerced appropriately.
	this->SetSize(this->GetSize());
	}

void ZOSWindow_Mac::Zoom()
	{
	ZOSWindow_Std::Zoom();
	}

void ZOSWindow_Mac::ForceOnScreen(bool inEnsureTitleBarAccessible, bool inEnsureSizeBoxAccessible)
	{
	ASSERTLOCKED();
	ZOSWindow::ForceOnScreen(inEnsureTitleBarAccessible, inEnsureSizeBoxAccessible);
	}

void ZOSWindow_Mac::BringFront(bool inMakeVisible)
	{
	ASSERTLOCKED();
	ZOSApp_Mac::sGet()->BringWindowFront(fWindowRef, inMakeVisible);
	this->Internal_ReportShownStateChange();
	this->Internal_ReportActiveChange();
	}

void ZOSWindow_Mac::GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	ASSERTLOCKED();
	::sGetContentInsets(fWindowRef, outTopLeftInset, outBottomRightInset);
	}

bool ZOSWindow_Mac::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();

	outSizeBoxSize.h = 15;
	outSizeBoxSize.v = 15;
	return true;

#if 0
// The region stuff is not what we need -- it returns the region that would be
// considered the size box, including the extra stuff in the bottom right border,
// which makes for a 22 pixel square. Plus, when the window is not active, the
// region is empty. I still need to be able to determine from the window proc
// whether the window *would* have a size box.
	if (::sGestalt_Appearance())
		{
		ZDCRgn theRgn;
		::GetWindowRegion(fWindowRef, kWindowGrowRgn, theRgn);
		outSizeBoxSize = theRgn.Bounds().Size();
		return true;
		}
	short windowKind = ::GetWindowKind(fWindowRef);
	if ((windowKind & kKindBits) == kZOSWindow_MacKind)
		{
		if (windowKind & kCallDrawGrowIconBit)
			{
			outSizeBoxSize.h = 15;
			outSizeBoxSize.v = 15;
			return true;
			}
		}
#endif
	return false;
	}

ZDCRgn ZOSWindow_Mac::GetVisibleContentRgn()
	{
	ASSERTLOCKED();
	return ZDCRgn(this->GetSize());
	}

void ZOSWindow_Mac::InvalidateRegion(const ZDCRgn& inBadRgn)
	{
	ASSERTLOCKED();

// Pick up any pending OS-level invalid region
	ZDCRgn theUpdateRgn;

	theUpdateRgn = ((WindowPeek_)fWindowRef)->updateRgn;
	::SetEmptyRgn(((WindowPeek_)fWindowRef)->updateRgn);

	if (theUpdateRgn)
		{
// The update rgn is kept in global coords, so we shift to zero-based window coords
		theUpdateRgn += ::sFromGlobal(fWindowRef, ZPoint::sZero);
		}

// Add in the bad region we were passed
	theUpdateRgn |= inBadRgn;

	ZMutexLocker lockerStructure(fMutex_Structure);
	if (fCanvas)
		fCanvas->InvalClip();
	this->Internal_RecordInvalidation(theUpdateRgn);
	}

void ZOSWindow_Mac::UpdateNow()
	{
	ASSERTLOCKED();
	this->Internal_ReportInvalidation();
	}

ZRef<ZDCCanvas> ZOSWindow_Mac::GetCanvas()
	{
	ASSERTLOCKED();
	if (!fCanvas)
		fCanvas = new ZDCCanvas_QD_Window(this, GetWindowPort(fWindowRef), &fMutex_MessageDispatch, &fMutex_Structure);
	return fCanvas;
	}

void ZOSWindow_Mac::AcquireCapture()
	{
	ASSERTLOCKED();
	ZOSApp_Mac::sGet()->AcquireCapture(this);
	}

void ZOSWindow_Mac::ReleaseCapture()
	{
	ASSERTLOCKED();
	ZOSApp_Mac::sGet()->ReleaseCapture(this);
	}

void ZOSWindow_Mac::GetNative(void* outNative)
	{
	ASSERTLOCKED();
	*reinterpret_cast<WindowRef*>(outNative) = fWindowRef;
	}

void ZOSWindow_Mac::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	fMenuBar = inMenuBar;
	ZOSApp_Mac::sGet()->MenuBarChanged(this);
	}

ZDragInitiator* ZOSWindow_Mac::CreateDragInitiator()
	{
	ASSERTLOCKED();
	DragInitiator* theDragInitiator = nil;
	try
		{
		theDragInitiator = new DragInitiator;
		}
	catch (...)
		{}
	return theDragInitiator;
	}

int32 ZOSWindow_Mac::GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone)
	{
	int32 mainIndex = -1;
	int32 count = 0;
	GDHandle currentGDHandle = ::GetDeviceList();
	while (currentGDHandle)
		{
		if (::TestDeviceAttribute(currentGDHandle, screenActive))
			{
			if (::TestDeviceAttribute(currentGDHandle, mainScreen))
				mainIndex = count;
			if (ZRect(currentGDHandle[0]->gdRect).Contains(inGlobalLocation))
				return count;
			++count;
			}
		currentGDHandle = ::GetNextDevice(currentGDHandle);
		}
	if (inReturnMainIfNone)
		return mainIndex;
	return -1;
	}

int32 ZOSWindow_Mac::GetScreenIndex(const ZRect& inGlobalRect, bool inReturnMainIfNone)
	{
	int32 maxArea = 0;
	int32 maxAreaIndex = -1;
	int32 mainIndex = -1;
	int32 count = 0;
	GDHandle currentGDHandle = ::GetDeviceList();
	while (currentGDHandle)
		{
		if (::TestDeviceAttribute(currentGDHandle, screenActive))
			{
// If this is the main screen, then make a note of it
			if (::TestDeviceAttribute(currentGDHandle, mainScreen))
				mainIndex = count;
			ZRect intersectedRect = inGlobalRect & ZRect(currentGDHandle[0]->gdRect);
			if (maxArea < intersectedRect.Width() * intersectedRect.Height())
				{
				maxArea = intersectedRect.Width() * intersectedRect.Height();
				maxAreaIndex = count;
				}
			++count;
			}
		currentGDHandle = ::GetNextDevice(currentGDHandle);
		}
	if (maxAreaIndex != -1)
		return maxAreaIndex;
	if (inReturnMainIfNone)
		return mainIndex;
	return -1;
	}

int32 ZOSWindow_Mac::GetMainScreenIndex()
	{
	int32 count = 0;
	GDHandle currentGDHandle = ::GetDeviceList();
	while (currentGDHandle)
		{
		if (::TestDeviceAttribute(currentGDHandle, screenActive))
			{
			if (::TestDeviceAttribute(currentGDHandle, mainScreen))
				return count;
			++count;
			}
		currentGDHandle = ::GetNextDevice(currentGDHandle);
		}
	ZDebugStopf(2, ("ZOSWindow_Mac::GetMainScreenIndex, no main screen found"));
	return -1;
	}

int32 ZOSWindow_Mac::GetScreenCount()
	{
	int32 count = 0;
	GDHandle currentGDHandle = ::GetDeviceList();
	while (currentGDHandle)
		{
		if (::TestDeviceAttribute(currentGDHandle, screenActive))
			++count;
		currentGDHandle = ::GetNextDevice(currentGDHandle);
		}
	return count;
	}

ZRect ZOSWindow_Mac::GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly)
	{
	ZRect theRect = ZRect::sZero;
	int32 count = 0;
	GDHandle currentGDHandle = ::GetDeviceList();
	while (currentGDHandle)
		{
		if (::TestDeviceAttribute(currentGDHandle, screenActive))
			{
			if (count == inScreenIndex)
				{
				theRect = currentGDHandle[0]->gdRect;
				if (inUsableSpaceOnly && ::TestDeviceAttribute(currentGDHandle, mainScreen))
					theRect.top += ::GetMBarHeight();
				break;
				}
			++count;
			}
		currentGDHandle = ::GetNextDevice(currentGDHandle);
		}
	return theRect;
	}

ZDCRgn ZOSWindow_Mac::GetDesktopRegion(bool inUsableSpaceOnly)
	{
	ZDCRgn theRgn; // Don't initialize from GetGrayRgn -- it'll take ownership of the RgnHandle
	theRgn = ::GetGrayRgn();
	if (!inUsableSpaceOnly)
		{
// Add back the menu bar space and any rounded corners.
		theRgn |= qd.screenBits.bounds;
		}
	return theRgn;
	}

ZDCRgn ZOSWindow_Mac::Internal_GetExcludeRgn()
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);
	return this->Internal_GetInvalidRgn();
	}

void ZOSWindow_Mac::Internal_TitleIconClick(const ZMessage& inMessage)
	{
	ASSERTLOCKED();
// Need to communicate with the app's event loop thread and tell it to initiate a drag of the window.
	}

void ZOSWindow_Mac::FinalizeCanvas(ZDCCanvas_QD_Window* inCanvas)
	{
	ZMutexLocker locker(fMutex_Structure);
	ZAssertStop(kDebug_Mac, inCanvas == fCanvas);

	if (fCanvas->GetRefCount() > 1)
		{
		fCanvas->FinalizationComplete();
		}
	else
		{
		fCanvas = nil;
		inCanvas->FinalizationComplete();
		delete inCanvas;
		}
	}

void ZOSWindow_Mac::Internal_SetLocation(ZPoint inLocation)
	{
	::MacMoveWindow(fWindowRef, inLocation.h, inLocation.v, false);
	this->Internal_RecordLocationChange(inLocation);
	}

void ZOSWindow_Mac::Internal_SetSize(ZPoint inSize)
	{
	inSize.h = min(inSize.h, fSize_Max.h);
	inSize.v = min(inSize.v, fSize_Max.v);
	inSize.h = max(inSize.h, fSize_Min.h);
	inSize.v = max(inSize.v, fSize_Min.v);
	ZPoint currentSize = ::sGetSize(fWindowRef);
	if (currentSize != inSize)
		{
		::SizeWindow(fWindowRef, inSize.h, inSize.v, true);
		this->Internal_RecordSizeChange(inSize);
		}
	}

short ZOSWindow_Mac::sCalcWindowKind(Layer inLayer, bool inCallDrawGrowIcon)
	{
	short theKind = kZOSWindow_MacKind;
	if (inCallDrawGrowIcon)
		theKind |= kCallDrawGrowIconBit;

	switch (inLayer)
		{
		case layerDummy: theKind |= kDummy; break;
		case layerSinker: theKind |= kSinker; break;
		case layerDocument: theKind |= kDocument; break;
		case layerFloater: theKind |= kFloater; break;
		case layerDialog: theKind |= kDialog; break;
		case layerMenu: theKind |= kMenu; break;
		case layerHelp: theKind |= kHelp; break;
		default:
			ZDebugStopf(1, ("ZOSWindow_Mac::sCalcWindowKind, didn't recognize the layer designator"));
		}
	return theKind;
	}

void ZOSWindow_Mac::sCalcWindowRefParams(const ZOSWindow::CreationAttributes& inAttributes, short& outProcID, bool& outCallDrawGrowIcon)
	{
	const bool gotAppearance = ::sGestalt_Appearance();
	const bool gotAppearance101 = gotAppearance && ::sGestalt_Appearance101();

	const bool gotTitleIconResource = ::sHasTitleIconResource();

	bool reallyResizable = inAttributes.fResizable || inAttributes.fHasSizeBox;

	switch (inAttributes.fLook)
		{
		case ZOSWindow::lookDocument:
			{
			if (!gotAppearance || (inAttributes.fHasTitleIcon && gotTitleIconResource && ZCONFIG(Processor, 68K)))
				{
				if (inAttributes.fResizable || inAttributes.fHasSizeBox)
					{
					if (inAttributes.fHasZoomBox)
						outProcID = zoomDocProc;
					else
						outProcID = documentProc;
					outCallDrawGrowIcon = true;
					}
				else
					{
					if (inAttributes.fHasZoomBox)
						outProcID = zoomNoGrow;
					else
						outProcID = noGrowDocProc;
					}
				if (inAttributes.fHasTitleIcon && gotTitleIconResource)
					outProcID += (6 << 4);
				}
			else
				{
				if (inAttributes.fResizable || inAttributes.fHasSizeBox)
					{
					if (inAttributes.fHasZoomBox)
						outProcID = kWindowFullZoomGrowDocumentProc;
					else
						outProcID = kWindowGrowDocumentProc;
					}
				else
					{
					if (inAttributes.fHasZoomBox)
						outProcID = kWindowFullZoomDocumentProc;
					else
						outProcID = kWindowDocumentProc;
					}
				}
			break;
			}
		case ZOSWindow::lookPalette:
			{
			if (gotAppearance)
				{
				if (inAttributes.fResizable || inAttributes.fHasSizeBox)
					{
					if (inAttributes.fHasZoomBox)
						outProcID = kWindowFloatFullZoomGrowProc;
					else
						outProcID = kWindowFloatGrowProc;
					}
				else
					{
					if (inAttributes.fHasZoomBox)
						outProcID = kWindowFloatFullZoomProc;
					else
						outProcID = kWindowFloatProc;
					}
				}
			else
				{
				if (inAttributes.fResizable || inAttributes.fHasSizeBox)
					{
					if (inAttributes.fHasZoomBox)
						outProcID = floatZoomGrowProc;
					else
						outProcID = floatGrowProc;
					outCallDrawGrowIcon = true;
					}
				else
					{
					if (inAttributes.fHasZoomBox)
						outProcID = floatZoomProc;
					else
						outProcID = floatProc;
					}
				}
			break;
			}
		case ZOSWindow::lookAlert:
			{
			outProcID = gotAppearance ? kWindowAlertProc : dBoxProc;
			reallyResizable = false;
			break;
			}
		case ZOSWindow::lookMovableAlert:
			{
			outProcID = gotAppearance ? kWindowMovableAlertProc : dBoxProc;
			reallyResizable = false;
			break;
			}
		case ZOSWindow::lookModal:
			{
			outProcID = gotAppearance ? kWindowModalDialogProc : dBoxProc;
			reallyResizable = false;
			break;
			}
		case ZOSWindow::lookMovableModal:
			{
			reallyResizable = false;
// AG 98-08-14. Appearance 1.0.1 has a resizable movable modal.
			if (gotAppearance101)
				{
				if (inAttributes.fResizable || inAttributes.fHasSizeBox)
					{
					outProcID = kWindowMovableModalGrowProc;
					reallyResizable = true;
					}
				else
					outProcID = kWindowMovableModalDialogProc;
				}
			else
				outProcID = gotAppearance ? kWindowMovableModalDialogProc : movableDBoxProc;
			break;
			}
		case ZOSWindow::lookThinBorder:
			{
			outProcID = gotAppearance ? kWindowPlainDialogProc : plainDBox;
			reallyResizable = false;
			break;
			}
		case ZOSWindow::lookMenu:
			{
			outProcID = gotAppearance ? kWindowPlainDialogProc : plainDBox;
			reallyResizable = false;
			break;
			}
		case ZOSWindow::lookHelp:
			{
			outProcID = kBalloonWDEFID << 4;
			reallyResizable = false;
			break;
			}
		default:
			ZUnimplemented();
		};
	}

WindowRef ZOSWindow_Mac::sCreateWindowRef(const ZRect& inInitialFrame, short inProcID, ZOSWindow::Layer inWindowLayer, bool inCallDrawGrowIcon, bool inHasCloseBox)
	{
	Rect tempRect = inInitialFrame;
	WindowRef theWindowRef = ::NewCWindow(nil, &tempRect, "\p", false, inProcID, ZOSWindow_Mac::sGetBehindPtr(inWindowLayer), inHasCloseBox, 0);
	if (theWindowRef == nil)
		throw bad_alloc();
	SetWindowKind(theWindowRef, ZOSWindow_Mac::sCalcWindowKind(inWindowLayer, inCallDrawGrowIcon));
	return theWindowRef;
	}

WindowRef ZOSWindow_Mac::sGetBehindPtr(Layer inLayer)
	{
// Find the window above the highest window in this layer (so that if the window
// stack is screwed we'll still end up in the right place)
	WindowRef previousWindow = (WindowRef)-1L;
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (MacIsWindowVisible(iterWindow))
			{
			if (sGetLayer(iterWindow) <= inLayer)
				break;
			}
		previousWindow = iterWindow;
		}
	return previousWindow;
	}

#if 0
// This needs reimplementation, using the kCallDrawGrowIconBit or window capaibilities API
bool ZOSWindow_Mac::sHasSizeBox(WindowRef inWindowRef)
	{
	short windowKind = GetWindowKind(inWindowRef);
	if ((windowKind & kKindBits) == kZOSWindow_MacKind)
		{
		if (windowKind & kCallDrawGrowIconBit)
			return true;
		}
	return false;
	}
#endif

ZOSWindow::Layer ZOSWindow_Mac::sGetLayer(WindowRef inWindowRef)
	{
	short windowKind = GetWindowKind(inWindowRef);
	if (windowKind == kDialogWindowKind)
		return layerDialog;
	if ((windowKind & kKindBits) == kZOSWindow_MacKind)
		{
		switch (windowKind & kLayerBits)
			{
			case kDummy: return layerDummy;
			case kSinker: return layerSinker;
			case kDocument: return layerDocument;
			case kFloater:
			case kHiddenFloater: return layerFloater;
			case kDialog: return layerDialog;
			case kMenu: return layerMenu;
			case kHelp: return layerHelp;
			}
		ZDebugStopf(1, ("ZOSWindow_Mac::sGetLayer, got a bad window kind"));
		}

	ZAssertLogf(1, windowKind >= 0, ("We've got a DA in our window list, somehow"));

// If we don't recognize it, then say it's a document
	return layerDocument;
	}

ZOSWindow_Mac* ZOSWindow_Mac::sFromWindowRef(WindowRef inWindowRef)
	{
	if (inWindowRef)
		{
		if ((GetWindowKind(inWindowRef) & kKindBits) == kZOSWindow_MacKind)
			return (ZOSWindow_Mac*)::GetWRefCon(inWindowRef);
		}
	return nil;
	}

ZPoint ZOSWindow_Mac::sFromGlobal(ZOSWindow_Mac* inWindow, ZPoint inGlobalPoint)
	{
	return ::sFromGlobal(inWindow->fWindowRef, inGlobalPoint);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Mac

ZOSApp_Mac* ZOSApp_Mac::sOSApp_Mac = nil;

static bool sInitedToolbox = false;

ZOSApp_Mac* ZOSApp_Mac::sGet()
	{ return sOSApp_Mac; }

ZOSApp_Mac::ZOSApp_Mac()
	{
	ZAssertStop(kDebug_Mac, sOSApp_Mac == nil);
	sOSApp_Mac = this;

	OSErr err;

// First thing, get the command line posted
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:CommandLine");
	this->QueueMessageForOwner(theMessage);

	fPostedRunStarted = false;

// Remember the app's resource file
	fAppResFile = ::CurResFile();

// Get the app's FSSpec
	fAppFSSpec.vRefNum = 0;
	fAppFSSpec.parID = 0;
	fAppFSSpec.name[0] = 0;

	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;
	ProcessInfoRec info;
	ZBlockZero(&info, sizeof(info));
	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processAppSpec = &fAppFSSpec;
	err = ::GetProcessInformation(&process, &info);

	if (!sInitedToolbox)
		{
// Now initialize the toolbox, if we have not already done so.
		sInitedToolbox = true;
		::InitGraf((Ptr) &qd.thePort);
		::InitFonts();
		::InitWindows();
		::InitMenus();
		::TEInit();
		::InitDialogs(nil);
		::InitCursor();

// Let the appearance manager know that we know it's there
		if (::sGestalt_Appearance())
			err = ::RegisterAppearanceClient();

// Install the AppleEvent handler for display manager events
		AEEventHandlerUPP theAEEventHandlerUPP = NewAEEventHandlerUPP(sAppleEventHandler);
		err =::AEInstallEventHandler(kCoreEventClass, kAESystemConfigNotice, theAEEventHandlerUPP, kAESystemConfigNotice, false);
// And the rest of the core suite
		err =::AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, theAEEventHandlerUPP, kAEOpenApplication, false);
		err =::AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, theAEEventHandlerUPP, kAEReopenApplication, false);
		err =::AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, theAEEventHandlerUPP, kAEOpenDocuments, false);
		err =::AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, theAEEventHandlerUPP, kAEPrintDocuments, false);
		err =::AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, theAEEventHandlerUPP, kAEQuitApplication, false);

// Install drag and drop handlers
		::InstallTrackingHandler(NewDragTrackingHandlerUPP(sDragTrackingHandler), nil, nil);
		::InstallReceiveHandler(NewDragReceiveHandlerUPP(sDragReceiveHandler), nil, nil);

// Get the apple menu built, it must be done this early otherwise none of the system menus get constructed either.
		MenuHandle appleMenuHandle = ::NewMenu(128, "\p\x14");
		::AppendResMenu(appleMenuHandle, 'DRVR'); // add DA names
		::InsertMenu(appleMenuHandle, 0);

		sInitedToolbox = true;
		}
	else
		{
// If we're constructing again, we won't receive app open or document open AEs, so queue the "run started" message
		fPostedRunStarted = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:RunStarted");
		this->QueueMessageForOwner(theMessage);
		}

// Set the scratch DC
	ZDCScratch::sSet(new ZDCCanvas_QD_OffScreen(ZPoint(2,1), ZDCPixmapNS::eFormatEfficient_Color_32));

// Set up window management stuff
	fSinkerLayerActive = false;
	fWindowRef_Dummy = nil;
	fWindowRef_DummyDialog = nil;
	fCount_DummyDialog = 0;

	fCCrsrHandle = nil;

	fAppleMenuItemCount = 0;

	fWindow_Active = nil;
	fWindow_Capture = nil;
	fWindow_ContainingMouse = nil;
	fWindow_LiveResize = nil;
	fWindow_LiveDrag = nil;

	fWindow_SetCursor = nil;
	fWindow_SetCursor_WasBusy = false;

	fDragActive = false;
	fDropOccurred = false;
	fLocation_LastDragDrop = ZPoint::sZero;

	fSemaphore_IdleExited = nil;
	fSemaphore_EventLoopExited = nil;

// Fire off the OS event loop thread
	ZMainNS::sInvokeAsMainThread(sRunEventLoop, this);

// And the idle thread
	(new ZThreadSimple<ZOSApp_Mac*>(sIdleThread, this))->Start();
	}

ZOSApp_Mac::~ZOSApp_Mac()
	{
	ZAssertLocked(kDebug_Mac, fMutex_MessageDispatch);

// We're being destroyed, kill all remaining windows, with extreme prejudice.
	ZMutexLocker structureLocker(fMutex_Structure, false);
	ZAssertStop(kDebug_Mac, structureLocker.IsOkay());
	ZMessage theDieMessage;
	theDieMessage.SetString("what", "zoolib:DieDieDie");

#if 1
// This version of the loop will post to *every* window, then wait for something to happen. The
// older version would post to the topmost window, wait, then post to the new topmost until
// the list went empty.
	while (true)
		{
		bool hitAny = false;
		for (WindowRef iterWindow = ::GetWindowList(); iterWindow != nil; iterWindow = MacGetNextWindow(iterWindow))
			{
			if (ZOSWindow_Mac* frontMostOSWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
				{
				frontMostOSWindow->QueueMessage(theDieMessage);
				hitAny = true;
				}
			}
		if (!hitAny)
			break;
		fCondition_Structure.Wait(fMutex_Structure);
		}
#else // 1
	while (true)
		{
		ZOSWindow_Mac* frontMostOSWindow = nil;
		for (WindowRef iterWindow = ::GetWindowList(); iterWindow != nil && frontMostOSWindow == nil; iterWindow = MacGetNextWindow(iterWindow))
			frontMostOSWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow);
		if (!frontMostOSWindow)
			break;
		frontMostOSWindow->QueueMessage(theDieMessage);
		fCondition_Structure.Wait(fMutex_Structure);
		}
#endif // 1

// Release the structure lock, which both our idle thread and our low level event thread will block on.
	structureLocker.Release();

// Tell our idle thread to exit, and wait for it to do so.
	ZSemaphore semaphore_IdleExited;
	fSemaphore_IdleExited = &semaphore_IdleExited;
	semaphore_IdleExited.Wait(1);

// Shut down the event loop by setting fSemaphore_EventLoopExited false and waking the process.
	ZSemaphore semaphore_EventLoopExited;
	fSemaphore_EventLoopExited = &semaphore_EventLoopExited;
	ProcessSerialNumber process;
	process.highLongOfPSN = 0;
	process.lowLongOfPSN = kCurrentProcess;
	::WakeUpProcess(&process);
// Wait for the event loop thread to exit
	semaphore_EventLoopExited.Wait(1);

	ZDCScratch::sSet(ZRef<ZDCCanvas>());

	ZAssertStop(kDebug_Mac, sOSApp_Mac == this);
	sOSApp_Mac = nil;

	if (fCCrsrHandle)
		::DisposeCCursor(fCCrsrHandle);
	}

// From ZOSApp via ZOSApp_Std
bool ZOSApp_Mac::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSApp_Std::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:MacMenuClick")
			{
			if (fOwner)
				{
				::InitCursor();
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				if (ZRef<ZMenuItem> theMenuItem = this->PostSetupAndHandleMenuBarClick(fMenuBar, inMessage.GetPoint("where")))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		else if (theWhat == "zoolib:MacMenuKey")
			{
			if (fOwner)
				{
				fMenuBar.Reset();
				fOwner->OwnerSetupMenus(fMenuBar);
				EventRecord theEventRecord;
				inMessage.GetRaw("eventRecord", &theEventRecord, sizeof(EventRecord));
				if (ZRef<ZMenuItem> theMenuItem = this->PostSetupAndHandleCammandKey(fMenuBar, theEventRecord))
					{
					ZMessage theMessage = theMenuItem->GetMessage();
					theMessage.SetString("what", "zoolib:Menu");
					theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		}
	return false;
	}

string ZOSApp_Mac::GetAppName()
	{ return ZString::sFromPString(fAppFSSpec.name); }

bool ZOSApp_Mac::HasUserAccessibleWindows()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (MacIsWindowVisible(iterWindow))
			{
			if (ZOSWindow_Mac* currentWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
				return true;
			}
		}
	return false;
	}

bool ZOSApp_Mac::HasGlobalMenuBar()
	{ return true; }

void ZOSApp_Mac::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	fMenuBar = inMenuBar;
	ZOSWindow_Mac* currentActiveWindow = this->Internal_GetActiveWindow();
	if (currentActiveWindow == nil)
		this->Internal_SetMenuBar(fMenuBar);
	}

void ZOSApp_Mac::BroadcastMessageToAllWindows(const ZMessage& inMessage)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (ZOSWindow_Std* currentWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
			{
			currentWindow->QueueMessage(inMessage);
			}
		}
	this->QueueMessage(inMessage);
	}

ZOSWindow* ZOSApp_Mac::CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes)
	{
	short macProcID;
	bool mustCallDrawGrowIcon;
	ZOSWindow_Mac::sCalcWindowRefParams(inAttributes, macProcID, mustCallDrawGrowIcon);

	ZOSWindow_Mac* theOSWindow = nil;
	WindowRef theWindowRef = nil;
	try
		{
		theWindowRef = ZOSWindow_Mac::sCreateWindowRef(inAttributes.fFrame, macProcID, inAttributes.fLayer, mustCallDrawGrowIcon, inAttributes.fHasCloseBox);
		theOSWindow = new ZOSWindow_Mac(theWindowRef);
		}
	catch (...)
		{
		if (theWindowRef)
			::DisposeWindow(theWindowRef);
		}
	return theOSWindow;
	}

bool ZOSApp_Mac::Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
												const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
												bigtime_t inTimeout, ZOSWindow_Std*& outWindow)
	{
	outWindow = nil;
	bigtime_t endTime = ZTicks::sNow() + inTimeout;
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (ZOSWindow_Std* currentWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
			{
			if (!ZUtil_STL::sContains(inVector_VisitedWindows, currentWindow))
				{
				if (!ZUtil_STL::sContains(inVector_DroppedWindows, currentWindow))
					{
					ZThread::Error err = currentWindow->GetLock().Acquire(endTime - ZTicks::sNow());
					if (err == ZThread::errorTimeout)
						return false;
					if (err == ZThread::errorNone)
						{
						outWindow = currentWindow;
						return true;
						}
					}
				}
			}
		}
	return true;
	}

ZOSWindow_Mac* ZOSApp_Mac::GetActiveOSWindow()
	{
	return this->Internal_GetActiveWindow();
	}

// Window management and OS event loop
void ZOSApp_Mac::AddOSWindow(ZOSWindow_Mac* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	fCondition_Structure.Broadcast();
	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_Mac::RemoveOSWindow(ZOSWindow_Mac* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	if (fWindow_Active == inOSWindow)
		fWindow_Active = nil;
	if (fWindow_Capture == inOSWindow)
		fWindow_Capture = nil;
	if (fWindow_ContainingMouse == inOSWindow)
		fWindow_ContainingMouse = nil;
	if (fWindow_Active == inOSWindow)
		fWindow_Active = nil;
	if (fWindow_SetCursor == inOSWindow)
		fWindow_SetCursor = nil;
	if (fWindow_LiveResize == inOSWindow)
		fWindow_LiveResize = nil;
	if (fWindow_LiveDrag == inOSWindow)
		fWindow_LiveDrag = nil;

	this->Internal_OSWindowRemoved(inOSWindow);

	fCondition_Structure.Broadcast();

	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_Mac::RunEventLoop()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	RgnHandle cursorRgn = ::NewRgn();
	while (!fSemaphore_EventLoopExited)
		{
		if (fDragActive)
			{		
			structureLocker.Release();
			ZThreadTM_Yield();
			structureLocker.Acquire();
			}
		else
			{	
			bool avoidWait = (fWindow_Capture != nil || fWindow_LiveResize != nil || fWindow_LiveDrag != nil);
			structureLocker.Release();
	
			Point mouse;
			::GetMouse(&mouse);
// To global coords (We *must* be in some port, mustn't we?)
			::LocalToGlobal(&mouse);
// Let ZThreadTM handle WaitNextEvent -- it will tune the sleep time to be both responsive and polite.
			EventRecord theEvent;
			::MacSetRectRgn(cursorRgn, mouse.h, mouse.v, mouse.h + 1, mouse.v + 1);
			ZThreadTM_WaitNextEvent(avoidWait, theEvent, cursorRgn);

			structureLocker.Acquire();
	
			if (!fSemaphore_EventLoopExited)
				{
				try
					{
					this->DispatchEvent(theEvent);
					}
				catch (...)
					{
					}
				}
			}
		}
	DisposeRgn(cursorRgn);
	fSemaphore_EventLoopExited->Signal(1);
	}

void ZOSApp_Mac::sRunEventLoop(void* inOSApp_Mac)
	{ reinterpret_cast<ZOSApp_Mac*>(inOSApp_Mac)->RunEventLoop(); }

void ZOSApp_Mac::DispatchEvent(const EventRecord& inEvent)
	{
	switch (inEvent.what)
		{
		case nullEvent:
			this->DispatchIdle(inEvent);
			break;
		case mouseUp:
			{
			WindowRef hitWindowRef;
			short thePart = ::FindWindow(inEvent.where, &hitWindowRef);
			this->DispatchMouseUp(inEvent, hitWindowRef, thePart);
			}
			break;
		case mouseDown:
			{
			WindowRef theWindowRef;
			short thePart = ::FindWindow(inEvent.where, &theWindowRef);
			switch (thePart)
				{
				case inSysWindow:
					{
					this->CancelCapture();
					::SystemClick(&inEvent, theWindowRef);
					break;
					}
				case inMenuBar:
					{
					this->CancelCapture();
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:MacMenuClick");
					theMessage.SetPoint("where", ZPoint(inEvent.where));
					if (ZOSWindow_Mac* activeWindow = this->Internal_GetActiveWindow())
						activeWindow->QueueMessage(theMessage);
					else
						this->QueueMessage(theMessage);
					break;
					}
				case inGoAway:
				case inContent:
				case inDrag:
				case inGrow:
				case inZoomIn:
				case inZoomOut:
				default:
					{
// All others, including whatever new codes may get defined
					this->DispatchMouseDown(inEvent, theWindowRef, thePart);
					break;
					}
				}
			break;
			}
		case autoKey:
		case keyDown:
			{
			if (fWindow_Capture == nil)
				{
				if ((inEvent.modifiers & cmdKey) != 0)
					{
					if (inEvent.what == keyDown)
						{
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:MacMenuKey");
						theMessage.SetRaw("eventRecord", &inEvent, sizeof(EventRecord));
//						theMessage.SetInt32("charCode", inEvent.message & charCodeMask);
//						theMessage.SetInt32("keyCode", (inEvent.message & keyCodeMask) >> 8);
//						theMessage.SetBool("shift", (inEvent.modifiers & shiftKey) == shiftKey);
//						theMessage.SetBool("command", (inEvent.modifiers & cmdKey) == cmdKey);
//						theMessage.SetBool("option", (inEvent.modifiers & optionKey) == optionKey);
//						theMessage.SetBool("control", (inEvent.modifiers & controlKey) == controlKey);
//						theMessage.SetBool("capsLock", (inEvent.modifiers & alphaLock) == alphaLock);
						if (ZOSWindow_Mac* activeWindow = this->Internal_GetActiveWindow())
							activeWindow->QueueMessage(theMessage);
						else
							this->QueueMessage(theMessage);
						}
					}
				else
					{
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:KeyDown");
					theMessage.SetInt32("charCode", inEvent.message & charCodeMask);
					theMessage.SetInt32("keyCode", (inEvent.message & keyCodeMask) >> 8);
					theMessage.SetBool("shift", (inEvent.modifiers & shiftKey) == shiftKey);
					theMessage.SetBool("command", (inEvent.modifiers & cmdKey) == cmdKey);
					theMessage.SetBool("option", (inEvent.modifiers & optionKey) == optionKey);
					theMessage.SetBool("control", (inEvent.modifiers & controlKey) == controlKey);
					theMessage.SetBool("capsLock", (inEvent.modifiers & alphaLock) == alphaLock);
					if (ZOSWindow_Mac* activeWindow = this->Internal_GetActiveWindow())
						activeWindow->QueueMessageForOwner(theMessage);
					else
						{
						// Post it to the app?
						}
					}
				}
			break;
			}
		case updateEvt:
			{
			this->DispatchUpdate(inEvent);
			break;
			}
		case activateEvt:
			{
// If we received an activate event then some unexpected window activation occurred (probably).
// So do a fixup to correct any problem that may have occurred
			this->FixupWindows();
			break;
			}
		case diskEvt:
			{
			if ((inEvent.message & 0xFFFF0000) != 0)
				{
				ZUtil_Mac_HL::sPreDialog();
				::DILoad();
				::DIBadMount(ZPoint(120,120), inEvent.message);
				::DIUnload();
				ZUtil_Mac_HL::sPostDialog();
				}
			break;
			}
		case kHighLevelEvent:
			{
			::AEProcessAppleEvent(&inEvent);
			break;
			}
		case osEvt:
			{
			switch ((inEvent.message >> 24) & 0x0FF)
				{
				case suspendResumeMessage:
					{
					this->DispatchSuspendResume(inEvent);
					break;
					}
				case mouseMovedMessage:
					{
					WindowRef hitWindowRef;
					short hitPart = ::FindWindow(inEvent.where, &hitWindowRef);
					ZOSWindow_Mac* hitWindow = nil;
					if (hitPart == inContent)
						hitWindow = ZOSWindow_Mac::sFromWindowRef(hitWindowRef);
					this->Internal_RecordMouseContainment(inEvent, hitWindow, hitPart);
					break;
					}
				}
			break;
			}
		}
	}

void ZOSApp_Mac::UpdateAllWindows()
	{

	}

void ZOSApp_Mac::DispatchMouseDown(const EventRecord& inEvent, WindowRef inHitWindowRef, short inWindowPart)
	{
	if (fWindow_Capture)
		{
		if (inWindowPart != inContent || inHitWindowRef != fWindow_Capture->fWindowRef)
			this->CancelCapture();
		}

	ZOSWindow_Mac* hitWindow = ZOSWindow_Mac::sFromWindowRef(inHitWindowRef);
	if (inWindowPart == inContent)
		{
		this->Internal_RecordMouseContainment(inEvent, hitWindow, inWindowPart);
		WindowRef topDialog = nil;
		WindowRef topDocument = nil;
		WindowRef topSinker = nil;
		for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
			{
			if (MacIsWindowVisible(iterWindow))
				{
				if (ZOSWindow_Mac::sGetLayer(iterWindow) == ZOSWindow::layerDialog && topDialog == nil)
					topDialog = iterWindow;
				if (ZOSWindow_Mac::sGetLayer(iterWindow) == ZOSWindow::layerDocument && topDocument == nil)
					topDocument = iterWindow;
				if (ZOSWindow_Mac::sGetLayer(iterWindow) == ZOSWindow::layerSinker && topSinker == nil)
					topSinker = iterWindow;
				}
			}
		if ((topDialog == nil) || (inHitWindowRef == topDialog))
			this->Internal_RecordMouseDown(inEvent, hitWindow);
		else
			::SysBeep(1);
		return;
		}

	this->Internal_RecordMouseContainment(inEvent, nil, inWindowPart);
	switch (inWindowPart)
		{
// ----------
		case inGrow:
			{
			if (inEvent.modifiers & optionKey)
				{
				fWindow_LiveResize = hitWindow;
				fSize_LiveResize = ::sGetSize(inHitWindowRef);
				fLocation_LiveResizeOrDrag = inEvent.where;
				}
			else
				{
				Rect resizeLimits;
				resizeLimits.left = hitWindow->fSize_Min.h;
				resizeLimits.top = hitWindow->fSize_Min.v;
				resizeLimits.right = hitWindow->fSize_Max.h;
				resizeLimits.bottom = hitWindow->fSize_Max.v;
				if (ZCONFIG_Processor == ZCONFIG_Processor_PPC && ::sGestalt_WindowManager() && ::sGestalt_OS86OrLater())
					{
// ResizeWindow is only supported in OS 8.6 or later, and then only on PPC.
					Rect newContentRect;
					if (::ResizeWindow(inHitWindowRef, inEvent.where, &resizeLimits, &newContentRect))
						hitWindow->Internal_RecordFrameChange(newContentRect);
					}
				else
					{
					long newSizeLong = ::GrowWindow(inHitWindowRef, inEvent.where, &resizeLimits);
					if (newSizeLong != 0)
						hitWindow->Internal_SetSize(*(reinterpret_cast<Point*>(&newSizeLong)));
					}
				}
			break;
			}
// ----------
		case inZoomIn:
		case inZoomOut:
			{
			if (::TrackBox(inHitWindowRef, inEvent.where, inWindowPart))
				hitWindow->Internal_RecordZoomBoxHit();
			break;
			}
// ----------
		case inGoAway:
			{
			if (::TrackGoAway(inHitWindowRef, inEvent.where))
				hitWindow->Internal_RecordCloseBoxHit();
			break;
			}
// ----------
		case inDrag:
			{
			bool handledIt = false;
			if (::sIsTitleIconWindow(inHitWindowRef))
				{
				TitleIconStateData** theStateData = ::sGetTitleIconStateData(inHitWindowRef);
				if (theStateData)
					{
					ZRect structureRect = ((WindowPeek_)inHitWindowRef)->strucRgn[0]->rgnBBox;
					structureRect.left += theStateData[0]->fLeftEdge;
					structureRect.right = structureRect.left + 16;
					structureRect.top += 2;
					structureRect.bottom = structureRect.top + 16;
					if (structureRect.Contains(inEvent.where))
						{
						handledIt = true;
						ZMessage theMessage;
						theMessage.SetString("what", "zoolib:TitleIcon");
						theMessage.SetPoint("where", sFromGlobal(inHitWindowRef, inEvent.where));
						hitWindow->QueueMessageForOwner(theMessage);
						}
					}
				}

			bool hitWindowIsDialog = (ZOSWindow_Mac::sGetLayer(inHitWindowRef) == ZOSWindow::layerDialog);
			if (!hitWindowIsDialog && !handledIt && (inEvent.modifiers & controlKey))
				{
				handledIt = true;
				this->SendWindowBack(inHitWindowRef);
				}

			if (!handledIt)
				{
				handledIt = true;
				bool bringFront = !hitWindowIsDialog && ((inEvent.modifiers & cmdKey) == 0);
				if (inEvent.modifiers & optionKey)
					{
					if (bringFront)
						this->BringWindowFront(inHitWindowRef, false);
					fWindow_LiveDrag = hitWindow;
					fLocation_LiveResizeOrDrag = inEvent.where;
					}
				else
					{
					this->HandleDragWindow(inHitWindowRef, inEvent.where, bringFront);
					}
				}
			break;
			}
// ----------
		case inProxyIcon:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:TitleIcon");
			theMessage.SetPoint("where", sFromGlobal(inHitWindowRef, inEvent.where));
			hitWindow->QueueMessageForOwner(theMessage);
			break;
			}
// ----------
		case inDesk:
		default:
			break;
		}
	}

void ZOSApp_Mac::DispatchMouseUp(const EventRecord& inEvent, WindowRef inHitWindowRef, short inWindowPart)
	{
	if (fWindow_LiveResize || fWindow_LiveDrag)
		{
		fWindow_LiveResize = nil;
		fWindow_LiveDrag = nil;
		return;
		}

	ZOSWindow_Mac* hitWindow = nil;
	if (inWindowPart == inContent)
		hitWindow = ZOSWindow_Mac::sFromWindowRef(inHitWindowRef);

	this->Internal_RecordMouseContainment(inEvent, hitWindow, inWindowPart);
	this->Internal_RecordMouseUp(inEvent, hitWindow);
	}

void ZOSApp_Mac::DispatchUpdate(const EventRecord& inEvent)
	{
	WindowRef theWindowRef = reinterpret_cast<WindowRef>(inEvent.message);

	ZDCRgn theUpdateRgn;

	theUpdateRgn = ((WindowPeek_)theWindowRef)->updateRgn;
	::SetEmptyRgn(((WindowPeek_)theWindowRef)->updateRgn);

	if (ZOSWindow_Mac* theWindow = ZOSWindow_Mac::sFromWindowRef(theWindowRef))
		{
// The update rgn is kept in global coords, so we shift to zero-based window coords
		ZPoint translation = sFromGlobal(theWindowRef, ZPoint::sZero);
		theUpdateRgn += translation;
		ZMutexLocker lockerStructure(theWindow->fMutex_Structure);
		if (theWindow->fCanvas)
			theWindow->fCanvas->InvalClip();
		theWindow->Internal_RecordInvalidation(theUpdateRgn);
		}
	}

void ZOSApp_Mac::DispatchIdle(const EventRecord& inEvent)
	{
//	if (fWindow_Capture)
		{
		WindowRef theWindowRef;
		short theHitPart = ::FindWindow(inEvent.where, &theWindowRef);
		ZOSWindow_Mac* hitWindow = nil;
		if (theHitPart == inContent)
			hitWindow = ZOSWindow_Mac::sFromWindowRef(theWindowRef);

		this->Internal_RecordMouseContainment(inEvent, hitWindow, theHitPart);
		}
	}

void ZOSApp_Mac::DispatchSuspendResume(const EventRecord& inEvent)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);
	bool isActive = ((inEvent.message & resumeFlag) != 0);
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (ZOSWindow_Mac* currentOSWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
			currentOSWindow->Internal_RecordAppActiveChange(isActive);
		}

// Because the Mac OS (oh so helpfully) hilites the frontmost window on a resume
// event we create a dummy window offscreen for the Mac OS to fuck with. This window is
// disposed when we resume (become active), created when we're suspended
	if (isActive)
		{
		this->Internal_RecordMouseContainment(inEvent, nil, inNoWindow);
		if (fWindowRef_Dummy)
			{
			::ShowHide(fWindowRef_Dummy, false);
			::DisposeWindow(fWindowRef_Dummy);
			}
		fWindowRef_Dummy = nil;
		this->FixupWindows();
		}
	else
		{
		this->Internal_RecordMouseContainment(inEvent, nil, inNoWindow);

		if (!fWindowRef_Dummy)
			{
			Rect tempRect = { 20000, 20000, 20005, 20005 };
//			Rect tempRect(ZOSWindow_Mac::sOffscreenRect(ZPoint(5,5)));
			fWindowRef_Dummy = ::NewCWindow(nil, &tempRect, "\pdummy", false, plainDBox, (WindowRef)-1L, false, 0);
			SetWindowKind(fWindowRef_Dummy, kZOSWindow_MacKind | ZOSWindow_Mac::kDialog);
			::ShowHide(fWindowRef_Dummy, true);
			}
		this->FixupWindows();
		}
	}

void ZOSApp_Mac::HandleDragWindow(WindowRef inWindowRef, ZPoint inHitPoint, bool inBringFront)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);

	if (::WaitMouseUp())
		{
		ZUtil_Mac_LL::PreserveCurrentPort thePCP;
		ZUtil_Mac_LL::SaveRestorePort theSRP;

		ZDCRgn grayRgn;
		grayRgn = ::GetGrayRgn();

// Set up the Window Manager port.
		CGrafPtr windowManagerPort;
		::GetCWMgrPort(&windowManagerPort);
		::SetGWorld(windowManagerPort, nil);
		::SetClip(grayRgn.GetRgnHandle());

// Find the topmost window in our window's layer
		WindowRef clipAboveWindowRef = inWindowRef;
		if (inBringFront)
			{
// Find the lowest window in the layer above this window, and find the top window in our layer
			WindowRef visibleTopOfThisLayer = nil;
			ZOSWindow::Layer thisLayer = ZOSWindow_Mac::sGetLayer(inWindowRef);
			for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
				{
				ZOSWindow::Layer iterLayer = ZOSWindow_Mac::sGetLayer(iterWindow);
				if (iterLayer == thisLayer)
					{
					if (MacIsWindowVisible(iterWindow) && visibleTopOfThisLayer == nil)
						visibleTopOfThisLayer = iterWindow;
					}
				}
			if (visibleTopOfThisLayer)
				clipAboveWindowRef = visibleTopOfThisLayer;
			}

		::ClipAbove(clipAboveWindowRef);

// Create a region to drag
		ZDCRgn dragRgn;
		::GetWindowStructureRgn(inWindowRef, dragRgn.GetRgnHandle());
// Drag the window around
		Rect dragRect = grayRgn.Bounds();
		long dragResult = ::DragGrayRgn(dragRgn.GetRgnHandle(), inHitPoint, &dragRect, &dragRect, noConstraint, nil);

		if ((dragResult != 0) && (dragResult != 0x80008000))
			{
			ZPoint delta = *(reinterpret_cast<Point*>(&dragResult));
			ZOSWindow_Mac::sFromWindowRef(inWindowRef)->Internal_SetLocation(sGetLocation(inWindowRef) + delta);
			}
		}
// Bring the window forward if the command key wasn't down
	if (inBringFront)
		this->BringWindowFront(inWindowRef, false);
	}

void ZOSApp_Mac::ShowWindow(ZOSWindow_Mac* inOSWindow, bool inShowOrHide)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	WindowRef theWindowRef = inOSWindow->fWindowRef;
	if (MacIsWindowVisible(theWindowRef) == inShowOrHide)
		return;
// If it's a floater, and we're in the background, then life is simple. We just need to set the window kind appropriately.
	if (ZOSWindow_Mac::sGetLayer(theWindowRef) == ZOSWindow::layerFloater && this->IsInBackground())
		{
		if (inShowOrHide)
			SetWindowKind(theWindowRef, kZOSWindow_MacKind | ZOSWindow_Mac::kHiddenFloater);
		else
			SetWindowKind(theWindowRef, kZOSWindow_MacKind | ZOSWindow_Mac::kFloater);
		return;
		}

	WindowRef windowToShow = nil;
	WindowRef windowToHide = nil;

	if (inShowOrHide)
		windowToShow = theWindowRef;
	else
		windowToHide = theWindowRef;

// Call the routine which will reorder out of order windows, optionally show
// a particular window and handle any activation issues
	this->Internal_FixupWindows(windowToShow, windowToHide);
	}

void ZOSApp_Mac::BringWindowFront(WindowRef inWindowRefToMove, bool inMakeVisible)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

// This routine moves inOSWindow so it is above the highest window in its layer.
	ZOSWindow::Layer windowToMoveLayer = ZOSWindow_Mac::sGetLayer(inWindowRefToMove);

	WindowRef windowRefAbove = (WindowRef)-1L;
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (MacIsWindowVisible(iterWindow))
			{
			if (ZOSWindow_Mac::sGetLayer(iterWindow) <= windowToMoveLayer)
				break;
			}
		windowRefAbove = iterWindow;
		}

	::sSendBehind(inWindowRefToMove, windowRefAbove);

	if (windowToMoveLayer == ZOSWindow::layerSinker)
		fSinkerLayerActive = true;
	if (windowToMoveLayer == ZOSWindow::layerDocument)
		fSinkerLayerActive = false;

	this->Internal_FixupWindows((inMakeVisible ? inWindowRefToMove : nil), nil);
	}

void ZOSApp_Mac::SendWindowBack(WindowRef inWindowRefToMove)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

// This routine moves inWindowRefToMove so it is the bottom window in its layer (so that it is above the
// highest window in the layer below)
	ZOSWindow::Layer windowToMoveLayer = ZOSWindow_Mac::sGetLayer(inWindowRefToMove);

	WindowRef windowRefAbove = nil;
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (MacIsWindowVisible(iterWindow))
			{
			ZOSWindow::Layer iterLayer = ZOSWindow_Mac::sGetLayer(iterWindow);
			if (iterLayer < windowToMoveLayer)
				break;
			if (iterLayer >= windowToMoveLayer)
				windowRefAbove = iterWindow;
			}
		}

	::sSendBehind(inWindowRefToMove, windowRefAbove);

	this->Internal_FixupWindows(nil, nil);
	}

bool ZOSApp_Mac::IsFrontProcess()
	{
	ProcessSerialNumber currentProcess;
	::GetCurrentProcess(&currentProcess);
	ProcessSerialNumber frontProcess;
	::GetFrontProcess(&frontProcess);
	Boolean isFront;
	::SameProcess(&currentProcess, &frontProcess, &isFront);
	return isFront;
	}

bool ZOSApp_Mac::IsInBackground()
	{
// AG 1999-10-28. The test for fWindowRef_Dummy is not always accurate -- the app does
// not always come up frontmost, and hence we get ourselves out of step with reality.

// AG 1999-11-01. IsFrontProcess gives us the wrong answer when we are in the midst
// of switching from front to back. So let's use the old method for now.
#if 1
// If we're in the background then our dummy window will be up
	return fWindowRef_Dummy != nil;
#else
// Instead we use IsFrontProcess, which calls the process manager to get the real skinny.
	return !this->IsFrontProcess();
#endif
	}

bool ZOSApp_Mac::IsTopModal()
	{
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		if (MacIsWindowVisible(iterWindow) && ZOSWindow_Mac::sGetLayer(iterWindow) == ZOSWindow::layerDialog)
			return true;
		}
	return false;
	}

void ZOSApp_Mac::PreDialog()
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	this->CancelCapture();

	if (fCount_DummyDialog++ == 0)
		{
		ZAssertStop(kDebug_Mac, fWindowRef_DummyDialog == nil);
// Work out a rectangle that guaranteed to be offscreen
		ZDCRgn grayRgn;
		grayRgn = ::GetGrayRgn();
		ZRect grayBounds = grayRgn.Bounds();
		Rect tempRect = ZRect(-5, -5, -1, -1) + grayBounds.TopLeft();
		fWindowRef_DummyDialog = ::NewCWindow(nil, &tempRect, "\pdummy 4 dialogs", false, plainDBox, (WindowRef)-1L, false, 0);
		SetWindowKind(fWindowRef_DummyDialog, kZOSWindow_MacKind | ZOSWindow_Mac::kDialog);
		::ShowHide(fWindowRef_DummyDialog, true);
		this->FixupWindows();
		}
	}

void ZOSApp_Mac::PostDialog()
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	if (--fCount_DummyDialog == 0)
		{
		ZAssertStop(kDebug_Mac, fWindowRef_DummyDialog != nil);
		::ShowHide(fWindowRef_DummyDialog, false);
		::DisposeWindow(fWindowRef_DummyDialog);
		fWindowRef_DummyDialog = nil;
		this->FixupWindows();
		}
	}

void ZOSApp_Mac::SendWindowBehind(WindowRef inWindowToMove, WindowRef inOtherWindow)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);

	if (inWindowToMove == nil)
		return;
	if (inOtherWindow == nil || inOtherWindow == (WindowRef)-1L)
		return;
	if (inWindowToMove == inOtherWindow)
		return;
// Moves inWindowToMove behind inOtherWindow iff inOtherWindow is in our layer
	if (ZOSWindow_Mac::sGetLayer(inWindowToMove) != ZOSWindow_Mac::sGetLayer(inOtherWindow))
		return;
	::sSendBehind(inWindowToMove, inOtherWindow);
	this->FixupWindows();
	}

void ZOSApp_Mac::FixupWindows()
	{ this->Internal_FixupWindows(nil, nil); }

void ZOSApp_Mac::AcquireCapture(ZOSWindow_Mac* inAcquireWindow)
	{
	fMutex_Structure.Acquire();
	if (fWindow_Capture != inAcquireWindow)
		{
		ZOSWindow_Mac* oldWindow = fWindow_Capture;
		fWindow_Capture = inAcquireWindow;
		if (oldWindow)
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:CaptureCancelled");
			oldWindow->QueueMessageForOwner(theMessage);
			}
		}
	fMutex_Structure.Release();
	}

void ZOSApp_Mac::ReleaseCapture(ZOSWindow_Mac* inReleaseWindow)
	{
	fMutex_Structure.Acquire();
	if (fWindow_Capture && fWindow_Capture == inReleaseWindow)
		{
		fWindow_Capture = nil;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:CaptureReleased");
		inReleaseWindow->QueueMessageForOwner(theMessage);
		}
	fMutex_Structure.Release();
	}

void ZOSApp_Mac::CancelCapture()
	{ this->AcquireCapture(nil); }

void ZOSApp_Mac::CursorChanged(ZOSWindow_Mac* inWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	if (inWindow == fWindow_SetCursor || inWindow == fWindow_Capture)
		this->Internal_SetCursor(inWindow->fCursor);
	}

void ZOSApp_Mac::MenuBarChanged(ZOSWindow_Mac* inWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	ZOSWindow_Mac* currentActiveWindow = this->Internal_GetActiveWindow();
	if (inWindow == currentActiveWindow)
		{
		if (currentActiveWindow)
			this->Internal_SetMenuBar(currentActiveWindow->fMenuBar);
		else
			this->Internal_SetMenuBar(fMenuBar);
		}
	}

ZRef<ZMenuItem> ZOSApp_Mac::PostSetupAndHandleMenuBarClick(const ZMenuBar& inMenuBar, ZPoint inHitPoint)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	ZRef<ZMenuItem> theItem = ZMenu_Mac::sPostSetupAndHandleMenuBarClick(inMenuBar, inHitPoint, fAppleMenuItemCount);
	::HiliteMenu(0);
	return theItem;
	}

ZRef<ZMenuItem> ZOSApp_Mac::PostSetupAndHandleCammandKey(const ZMenuBar& inMenuBar, const EventRecord& inEventRecord)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	ZRef<ZMenuItem> theItem = ZMenu_Mac::sPostSetupAndHandleCommandKey(inMenuBar, inEventRecord, fAppleMenuItemCount);
	::HiliteMenu(0);
	return theItem;
	}

ZOSWindow_Mac* ZOSApp_Mac::Internal_GetActiveWindow()
	{ return fWindow_Active; }

void ZOSApp_Mac::Internal_SetCursor(const ZCursor& inCursor)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);

	if (inCursor.IsCustom())
		{
		ZPoint theHotSpot;
		ZDCPixmap colorPixmap, monoPixmap, maskPixmap;
		inCursor.Get(colorPixmap, monoPixmap, maskPixmap, theHotSpot);

		// We insist on having a mask and either a color or a mono cursor.
		ZAssertStop(kDebug_Mac, maskPixmap && (colorPixmap || monoPixmap));

		CCrsrHandle theCursorHandle = nil;
		if (colorPixmap)
			{
			theCursorHandle = CCrsrHandle(::NewHandle(sizeof(CCrsr)));
			ZUtil_Mac_LL::HandleLocker locker1((Handle)theCursorHandle);
			theCursorHandle[0]->crsrType = 0x8001;
			theCursorHandle[0]->crsrMap = (PixMapHandle)::NewHandle(sizeof(PixMap));
			ZUtil_Mac_LL::HandleLocker locker2(Handle(theCursorHandle[0]->crsrMap));
			ZUtil_Mac_LL::sSetupPixMapColor(ZPoint(16, 16), 16, theCursorHandle[0]->crsrMap[0][0]);
			theCursorHandle[0]->crsrData = ::NewHandleClear(2 * 16 * 16);
			ZUtil_Mac_LL::HandleLocker locker3(Handle(theCursorHandle[0]->crsrData));

			ZDCPixmapNS::RasterDesc theRasterDesc;
			theRasterDesc.fPixvalDesc.fDepth = 16;
			theRasterDesc.fPixvalDesc.fBigEndian = true;
			theRasterDesc.fRowBytes = 2 * 16;
			theRasterDesc.fRowCount = 16;
			theRasterDesc.fFlipped = false;

			colorPixmap.CopyTo(ZPoint(0, 0),
								theCursorHandle[0]->crsrData[0],
								theRasterDesc,
								ZDCPixmapNS::PixelDesc(0x7C00, 0x03E0, 0x001F, 0),
								ZRect(16, 16));

			theCursorHandle[0]->crsrXData = ::NewHandle(2);
			theCursorHandle[0]->crsrXValid = 0;
			theCursorHandle[0]->crsrXHandle = nil;
			theCursorHandle[0]->crsrHotSpot = theHotSpot;
			theCursorHandle[0]->crsrXTable = 0;
			theCursorHandle[0]->crsrID = ::GetCTSeed();

			// Now move the mono stuff over.
			theRasterDesc.fPixvalDesc.fDepth = 1;
			theRasterDesc.fPixvalDesc.fBigEndian = true;
			theRasterDesc.fRowBytes = 2;
			theRasterDesc.fRowCount = 16;
			theRasterDesc.fFlipped = false;
			ZDCPixmapNS::PixelDesc thePixelDesc(1, 0);
			if (monoPixmap)
				{
				// We have a mono pixmap, so we can use it for the mono version
				monoPixmap.CopyTo(ZPoint(0, 0), &theCursorHandle[0]->crsr1Data, theRasterDesc, thePixelDesc, ZRect(16, 16));
				}
			else
				{
				// Have to use a mono-ized version of the color pixmap.
				colorPixmap.CopyTo(ZPoint(0, 0), &theCursorHandle[0]->crsr1Data, theRasterDesc, thePixelDesc, ZRect(16, 16));
				}

			// Move the mask over
			maskPixmap.CopyTo(ZPoint(0, 0), &theCursorHandle[0]->crsrMask, theRasterDesc, thePixelDesc, ZRect(16, 16));

			::SetCCursor(theCursorHandle);
			}
		else
			{
			Cursor theMacCursor;
			ZBlockZero(&theMacCursor.data, sizeof(theMacCursor.data));
			ZBlockZero(&theMacCursor.mask, sizeof(theMacCursor.mask));

			ZDCPixmapNS::RasterDesc theRasterDesc;
			theRasterDesc.fPixvalDesc.fDepth = 1;
			theRasterDesc.fPixvalDesc.fBigEndian = true;
			theRasterDesc.fRowBytes = 2;
			theRasterDesc.fRowCount = 16;
			theRasterDesc.fFlipped = false;
			ZDCPixmapNS::PixelDesc thePixelDesc(1, 0);
			monoPixmap.CopyTo(ZPoint(0, 0), &theMacCursor.data, theRasterDesc, thePixelDesc, ZRect(16, 16));
			maskPixmap.CopyTo(ZPoint(0, 0), &theMacCursor.mask, theRasterDesc, thePixelDesc, ZRect(16, 16));
			theMacCursor.hotSpot = theHotSpot;
			::SetCursor(&theMacCursor);
			}
		if (fCCrsrHandle)
			::DisposeCCursor(fCCrsrHandle);
		fCCrsrHandle = theCursorHandle;
		}
	else
		{
#if ZCONFIG(Processor, PPC)
		if (::sGestalt_Appearance11())
			{
			UInt32 themeCursor = kThemeArrowCursor;
			switch (inCursor.GetCursorType())
				{
				case ZCursor::eCursorTypeArrow: themeCursor = kThemeArrowCursor; break;
				case ZCursor::eCursorTypeIBeam: themeCursor = kThemeIBeamCursor; break;
				case ZCursor::eCursorTypeCross: themeCursor = kThemeCrossCursor; break;
				case ZCursor::eCursorTypePlus: themeCursor = kThemePlusCursor; break;
				case ZCursor::eCursorTypeWatch: themeCursor = kThemeWatchCursor; break;
				}
			::SetThemeCursor(themeCursor);
			}
		else
#endif // ZCONFIG(Processor, PPC)
			{
			if (inCursor.GetCursorType() == ZCursor::eCursorTypeArrow)
				::InitCursor();
//				::SetCursor(&qd.arrow);
			else
				{
				short theResID = 0;
				CursHandle theCursHandle = nil;
				switch (inCursor.GetCursorType())
					{
					case ZCursor::eCursorTypeIBeam: theResID = iBeamCursor; break;
					case ZCursor::eCursorTypeCross: theResID = crossCursor; break;
					case ZCursor::eCursorTypePlus: theResID = plusCursor; break;
					case ZCursor::eCursorTypeWatch: theResID = watchCursor; break;
					}
				if (theResID)
					{
					CursHandle theCursHandle = ::MacGetCursor(theResID);
					if (theCursHandle)
						{
						ZUtil_Mac_LL::HandleLocker locker((Handle)theCursHandle);
						::SetCursor(*theCursHandle);
						}
					}
				}
			}
		}
	}

void ZOSApp_Mac::Internal_SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ZMenu_Mac::sInstallMenuBar(inMenuBar);
	}

static void sStuffMouseState(const EventRecord& inEvent, ZOSWindow_Mac* inWindow, ZOSWindow_Mac* inWindow_ContainingMouse, ZOSWindow_Mac* inWindow_Capture, ZOSWindow_Std::MouseState& outState)
	{
	outState.fLocation = ZOSWindow_Mac::sFromGlobal(inWindow, inEvent.where);
	outState.fContained = (inWindow == inWindow_ContainingMouse);
	outState.fCaptured = (inWindow == inWindow_Capture);

// Waddya know, btnState is the inverse of the mouse button state.
	outState.fButtons[0] = (inEvent.modifiers & btnState) == 0;
	outState.fButtons[1] = false;
	outState.fButtons[2] = false;
	outState.fButtons[3] = false;
	outState.fButtons[4] = false;

	outState.fModifier_Command = (inEvent.modifiers & cmdKey) != 0;
	outState.fModifier_Option = (inEvent.modifiers & optionKey) != 0;
	outState.fModifier_Shift = (inEvent.modifiers & shiftKey) != 0;
	outState.fModifier_Control = (inEvent.modifiers & controlKey) != 0;
	outState.fModifier_CapsLock = (inEvent.modifiers & alphaLock) != 0;
	}

void ZOSApp_Mac::Internal_RecordMouseDown(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse)
	{
	if (fWindow_Capture)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, fWindow_Capture, inWindow_ContainingMouse, fWindow_Capture, theState);
		fWindow_Capture->Internal_RecordMouseDown(0, theState, bigtime_t(inEvent.when) * 16667LL);
		}
	else if (inWindow_ContainingMouse)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, inWindow_ContainingMouse, inWindow_ContainingMouse, fWindow_Capture, theState);
		inWindow_ContainingMouse->Internal_RecordMouseDown(0, theState, bigtime_t(inEvent.when) * 16667LL);
		}
	}

void ZOSApp_Mac::Internal_RecordMouseUp(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse)
	{
	if (fWindow_Capture)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, fWindow_Capture, inWindow_ContainingMouse, fWindow_Capture, theState);
		fWindow_Capture->Internal_RecordMouseUp(theState, bigtime_t(inEvent.when) * 16667LL);
		}
	else if (inWindow_ContainingMouse)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, inWindow_ContainingMouse, inWindow_ContainingMouse, fWindow_Capture, theState);
		inWindow_ContainingMouse->Internal_RecordMouseUp(theState, bigtime_t(inEvent.when) * 16667LL);
		}
	}

void ZOSApp_Mac::Internal_RecordMouseContainment(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse, short inHitPart)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);
// If there's a drag/drop active, just bail
	if (fDragActive)
		return;

// If there is a live resize or drag in progress, act as if no window contains the mouse
	if (fWindow_LiveResize || fWindow_LiveDrag)
		inWindow_ContainingMouse = nil;

// Inform the last containing window (if any) of state change (if any)
	if (fWindow_ContainingMouse)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, fWindow_ContainingMouse, inWindow_ContainingMouse, fWindow_Capture, theState);
		fWindow_ContainingMouse->Internal_RecordMouseState(theState, bigtime_t(inEvent.when) * 16667LL);
		}

	if (fWindow_Capture)
		{
		ZOSWindow_Std::MouseState theState;
		::sStuffMouseState(inEvent, fWindow_Capture, inWindow_ContainingMouse, fWindow_Capture, theState);
		fWindow_Capture->Internal_RecordMouseState(theState, bigtime_t(inEvent.when) * 16667LL);
		}
	else if (inWindow_ContainingMouse)
		{
		if (fWindow_ContainingMouse != inWindow_ContainingMouse)
			{
			ZOSWindow_Std::MouseState theState;
			::sStuffMouseState(inEvent, inWindow_ContainingMouse, inWindow_ContainingMouse, fWindow_Capture, theState);
			inWindow_ContainingMouse->Internal_RecordMouseState(theState, bigtime_t(inEvent.when) * 16667LL);
			}
		}

	fWindow_ContainingMouse = inWindow_ContainingMouse;

// Update our notion of which window is responsible for setting the cursor. It will be the capture window, or the physically containing window.
	ZOSWindow_Mac* oldWindow_SetCursor = fWindow_SetCursor;

	fWindow_SetCursor = fWindow_ContainingMouse;
	if (fWindow_Capture)
		fWindow_SetCursor = fWindow_Capture;

	bool isBusy = false;
	if (fWindow_SetCursor)
		isBusy = fWindow_SetCursor->IsBusy();
	else if (fWindow_Active && inHitPart == inMenuBar)
		isBusy = fWindow_Active->IsBusy();

	if (fWindow_SetCursor != oldWindow_SetCursor || fWindow_SetCursor_WasBusy != isBusy)
		{
		fWindow_SetCursor_WasBusy = isBusy;
		if (isBusy)
			{
			ZCursor theCursor(ZCursor::eCursorTypeWatch);
			this->Internal_SetCursor(theCursor);
			}
		else
			{
			if (fWindow_SetCursor)
				this->Internal_SetCursor(fWindow_SetCursor->fCursor);
			else
				::InitCursor();
			}
		}

// If there's a live drag or resize active, then do the right thing.
	if (fWindow_LiveResize)
		{
		ZPoint location = inEvent.where;
		fWindow_LiveResize->Internal_SetSize(fSize_LiveResize + (location - fLocation_LiveResizeOrDrag));
		}

	if (fWindow_LiveDrag)
		{
		ZPoint location = inEvent.where;
		if (fLocation_LiveResizeOrDrag != location)
			{
			fWindow_LiveDrag->Internal_SetLocation(::sGetLocation(fWindow_LiveDrag->fWindowRef) + (location - fLocation_LiveResizeOrDrag));
			fLocation_LiveResizeOrDrag = location;
			}
		}
	}

void ZOSApp_Mac::Internal_SetShown(WindowRef inWindowRef, bool inShown)
	{
	if (MacIsWindowVisible(inWindowRef) != inShown)
		{
		::ShowHide(inWindowRef, inShown);

		if (ZOSWindow_Mac* theMacWindow = ZOSWindow_Mac::sFromWindowRef(inWindowRef))
			theMacWindow->Internal_RecordShownStateChange(inShown ? eZShownStateShown : eZShownStateHidden);

		this->Internal_RecordWindowRosterChange();
		}
	}

void ZOSApp_Mac::Internal_SetActive(WindowRef inWindowRef, bool inActive)
	{
	if (IsWindowHilited(inWindowRef) != inActive)
		::HiliteWindow(inWindowRef, inActive);

	if (ZOSWindow_Mac* theMacWindow = ZOSWindow_Mac::sFromWindowRef(inWindowRef))
		{
		if (theMacWindow->fWindowActive != inActive)
			{
//##			if (theMacWindow->HasSizeBox())
//##				theMacWindow->InvalidateSizeBoxRgn();
			theMacWindow->fWindowActive = inActive;
			theMacWindow->Internal_RecordActiveChange(inActive);
			}
		}
	}

void ZOSApp_Mac::Internal_FixupWindows(WindowRef windowToShow, WindowRef windowToHide)
	{
	ZAssertLocked(kDebug_Mac, fMutex_Structure);

// Stick the visible window list into a vector of WindowRef
	vector<WindowRef> windowVector;
	for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
		{
		short windowKind = GetWindowKind(iterWindow);

// If we're in the background, and this is a visible floater then hide it and mark it as such
		if (this->IsInBackground() &&
			(windowKind & ZOSWindow_Mac::kKindBits) == kZOSWindow_MacKind &&
			(windowKind & ZOSWindow_Mac::kLayerBits) == ZOSWindow_Mac::kFloater)
			{
			if (MacIsWindowVisible(iterWindow))
				SetWindowKind(iterWindow, kZOSWindow_MacKind | ZOSWindow_Mac::kHiddenFloater);
			::ShowHide(iterWindow, false);
			}
		else
			{
// If the window is visible, or it's an invisible floater that should be made visible, or
// it's windowToShow (which needs to be made visible), then stick it on the window vector
			if (MacIsWindowVisible(iterWindow) ||
				(iterWindow == windowToShow) || (iterWindow == windowToHide) ||
				((windowKind & ZOSWindow_Mac::kKindBits) == kZOSWindow_MacKind &&
				(windowKind & ZOSWindow_Mac::kLayerBits) == ZOSWindow_Mac::kHiddenFloater))
				{
				if (iterWindow != fWindowRef_Dummy)
					windowVector.push_back(iterWindow);
				}
			}
		}

#if 0
// AG 97-09-13. stable_sort adds about 30K of object code to this file. So
// instead we build the list by several passes. Slower, but *much* smaller
	struct CompareWindowRefs
		{
		bool operator()(const WindowRef& a, const WindowRef& b)
			{
			if (ZOSWindow_Mac::sGetLayer(a) > ZOSWindow_Mac::sGetLayer(b))
				return true;
			return false;
			}
		};
// Get a sorted version of the window list.
	::stable_sort(windowVector.begin(), windowVector.end(), CompareWindowRefs());
#else
	vector<WindowRef> tempVector;
	for (long currentLayer = ZOSWindow::layerTopmost; currentLayer >= ZOSWindow::layerBottommost; --currentLayer)
		{
		for (vector<WindowRef>::iterator i = windowVector.begin(); i != windowVector.end(); ++i)
			{
			if (ZOSWindow_Mac::sGetLayer(*i) == currentLayer)
				tempVector.push_back(*i);
			}
		}
	windowVector = tempVector;
#endif

	WindowRef previousWindow = (WindowRef)-1L;
	if (fWindowRef_Dummy)
		previousWindow = fWindowRef_Dummy;

// First pass -- restore the window order, and find the topmost window in each layer
	WindowRef topDialog = nil;
	WindowRef topDocument = nil;
	WindowRef topSinker = nil;

	for (vector<WindowRef>::iterator i = windowVector.begin(); i != windowVector.end(); ++i)
		{
		if (*i == windowToHide)
			this->Internal_SetShown(*i, false);
		else
// If it's the windowToShow, then show it now, otherwise sSendBehind will futz the
// window ordering (SendBehind does not keep the window ordering if the upper window is invisible)
			{
			if (*i == windowToShow)
				this->Internal_SetShown(*i, true);

			::sSendBehind(*i, previousWindow);
			previousWindow = *i;

			if (ZOSWindow_Mac::sGetLayer(*i) == ZOSWindow::layerDialog && topDialog == nil)
				topDialog = *i;
			if (ZOSWindow_Mac::sGetLayer(*i) == ZOSWindow::layerDocument && topDocument == nil)
				topDocument = *i;
			if (ZOSWindow_Mac::sGetLayer(*i) == ZOSWindow::layerSinker && topSinker == nil)
				topSinker = *i;
			}
		}

// If we don't have a window in the document/sinker layers that is active
// then activate the other layer
	if (!fSinkerLayerActive && topDocument == nil)
		fSinkerLayerActive = true;
	if (fSinkerLayerActive && topSinker == nil)
		fSinkerLayerActive = false;

// Remember which window is active (never a floater)
	ZOSWindow_Mac* oldActiveWindow = fWindow_Active;
	if (topDialog)
		fWindow_Active = ZOSWindow_Mac::sFromWindowRef(topDialog);
	else if (fSinkerLayerActive)
		fWindow_Active = ZOSWindow_Mac::sFromWindowRef(topSinker);
	else
		fWindow_Active = ZOSWindow_Mac::sFromWindowRef(topDocument);

// Second pass, hilite the windows, and show any floaters that were inappropriately hidden
	for (vector<WindowRef>::iterator i = windowVector.begin(); i != windowVector.end(); ++i)
		{
		short windowKind = GetWindowKind(*i);

		short fullLayer = ZOSWindow_Mac::kDocument;
		if (windowKind == kDialogWindowKind)
			fullLayer = ZOSWindow_Mac::kDialog;
		else if ((windowKind & ZOSWindow_Mac::kKindBits) == kZOSWindow_MacKind)
			fullLayer = windowKind & ZOSWindow_Mac::kLayerBits;
		else
			ZAssertLogf(1, windowKind >= 0, ("We've got a DA in our window list, somehow"));

// Note that we ignore layerDummy windows -- the only one currently used is used by the scrach DC created by ZOSApp_Mac
		if (fullLayer == ZOSWindow_Mac::kDialog)
			this->Internal_SetActive(*i, topDialog == *i && !this->IsInBackground());
		else if (fullLayer == ZOSWindow_Mac::kDocument)
			this->Internal_SetActive(*i, !this->IsInBackground() && topDialog == nil && !fSinkerLayerActive && topDocument == *i);
		else if (fullLayer == ZOSWindow_Mac::kSinker)
			this->Internal_SetActive(*i, !this->IsInBackground() && topDialog == nil && fSinkerLayerActive && topSinker == *i);
		else if (fullLayer == ZOSWindow_Mac::kFloater)
			this->Internal_SetActive(*i, !this->IsInBackground() && topDialog == nil);
		else if (fullLayer == ZOSWindow_Mac::kHiddenFloater)
			{
			if (!this->IsInBackground())
				{
				::ShowHide(*i, true);
				SetWindowKind(*i, kZOSWindow_MacKind | ZOSWindow_Mac::kFloater);
				}
			this->Internal_SetActive(*i, !this->IsInBackground() && topDialog == nil);
			}
		else if (fullLayer == ZOSWindow_Mac::kMenu)
			this->Internal_SetActive(*i, false);
		else if (fullLayer == ZOSWindow_Mac::kHelp)
			this->Internal_SetActive(*i, false);
		}

	if (oldActiveWindow != fWindow_Active)
		{
		if (fWindow_Active)
			this->Internal_SetMenuBar(fWindow_Active->fMenuBar);
		else
			this->Internal_SetMenuBar(fMenuBar);
		}
	}

static OSErr sGotRequiredParameters(const AppleEvent& inMessage)
	{
// This routine is here for convenience. To do AppleEvent processing "right", you really want to check that
// you have everything the sender sent you. Almost every good AppleEvent sample will have this routine and
// will call it from within the handlers. Since all handling is done from within an ZAppleObject (?),
// it makes sense for this to be a member function of ZAppleObject. However, the member function
// really doesn't need access to the object itself, so it is static (ie: it can be called from anywhere).

// Look for the keyMissedKeywordAttr, just to see if it's there
	DescType returnedType;
	Size actualSize;
	OSErr theErr = AEGetAttributePtr(&inMessage, keyMissedKeywordAttr,
						typeWildCard, &returnedType, nil, 0,
						&actualSize);

	if (theErr == errAEDescNotFound)
		{
		// The only error that is OK is to say that the descriptor was not
		// found (indicating that we got all the parameters)
		return noErr;
		}
	else if (theErr == noErr)
		return errAEParamMissed;
	return theErr;
	}

DEFINE_API(OSErr) ZOSApp_Mac::sAppleEventHandler(const AppleEvent* inMessage, AppleEvent* outReply, long inRefCon)
	{
	ZAssertStop(kDebug_Mac, ZOSApp_Mac::sGet());
	return ZOSApp_Mac::sGet()->AppleEventHandler(inMessage, outReply, inRefCon);
	}

OSErr ZOSApp_Mac::AppleEventHandler(const AppleEvent* inMessage, AppleEvent* outReply, unsigned long inRefCon)
	{
	switch (inRefCon)
		{
		case kAESystemConfigNotice:
			{
			ZMutexLocker structureLocker(fMutex_Structure);
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:ScreenLayoutChanged");
			for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
				{
				if (ZOSWindow_Mac* currentOSWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
					currentOSWindow->QueueMessageForOwner(theMessage);
				}
			break;
			}
		case kAEOpenApplication:
			{
			ZAssertStop(kDebug_Mac, !fPostedRunStarted);
			fPostedRunStarted = true;
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RunStarted");
			this->QueueMessageForOwner(theMessage);
			break;
			}
		case kAEReopenApplication:
			{
			break;
			}
		case kAEOpenDocuments:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:OpenDocument");
			this->QueueMessageForOwner(theMessage);

			if (!fPostedRunStarted)
				{
				fPostedRunStarted = true;
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:RunStarted");
				this->QueueMessageForOwner(theMessage);
				}
			break;
			}
		case kAEPrintDocuments:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:PrintDocument");
			this->QueueMessageForOwner(theMessage);

			if (!fPostedRunStarted)
				{
				fPostedRunStarted = true;
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:RunStarted");
				this->QueueMessageForOwner(theMessage);
				}
			break;
			}
		case kAEQuitApplication:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RequestQuit");
			this->QueueMessageForOwner(theMessage);
			break;
			}
		default:
			{
			ZDebugStopf(2, ("Unexpected AppleEvent received"));
			break;
			}
		}
	return noErr;
	}

#if 0
void ZOSApp_Mac::HandleAEOpenDocument(const AppleEvent& message, AppleEvent& reply)
	{
	AEDescList listOfFiles;

	if (noErr != ::AEGetParamDesc(&message, keyDirectObject, typeAEList, &listOfFiles))
		return;

	long numFiles;
	if (noErr == ::AECountItems(&listOfFiles, &numFiles))
		{
		for (long i = 1; i <= numFiles; i++)
			{
			DescType actualType;
			FSSpec theFileSpec;
			AEKeyword theActualKey;
			Size actualSize;

			if (noErr == ::AEGetNthPtr(&listOfFiles, i, typeFSS, &theActualKey,
								&actualType, (Ptr) &theFileSpec,
								sizeof(theFileSpec), &actualSize))
				{
				try
					{
					ZAppleObject::sGotRequiredParameters(message);
					this->DoAEOpen(theFileSpec, message, reply);
					}
				catch (...)
					{}
				}
			}
		OSErr theErr = ::AEDisposeDesc(&listOfFiles);
		}

//##	if (fState == eState_StartingRun)
//##		{
//##		fState = eState_Running;
//##		this->ReadyToRun();
//##		}
	}

void ZOSApp_Mac::HandleAEPrint(const AppleEvent& message, AppleEvent& reply)
	{
// Don't handle printing
	}

void ZOSApp_Mac::AppleEventIdleProc(EventRecord& theEventRecord, long& sleepTime, ZDCRgn& mouseRgn)
	{
	try
		{
//##		ZOSApp_Mac::sGet()->DispatchEvent(theEventRecord);
		}
	catch (...)
		{}
	}

bool ZOSApp_Mac::ModalFilter(DialogPtr theDialog, EventRecord& theEventRecord, short& itemHit)
	{
	ZUnimplemented();
	return false;
	}

#endif

DEFINE_API(OSErr) ZOSApp_Mac::sDragTrackingHandler(DragTrackingMessage inMessage, WindowRef inWindowRef, void* inRefCon, DragReference inDragReference)
	{ return ZOSApp_Mac::sGet()->DragTrackingHandler(inMessage, inWindowRef, inRefCon, inDragReference); }

OSErr ZOSApp_Mac::DragTrackingHandler(DragTrackingMessage inMessage, WindowRef inWindowRef, void* inRefCon, DragReference inDragReference)
	{
	ZOSWindow_Mac* theOSWindow = ZOSWindow_Mac::sFromWindowRef(inWindowRef);
	if (theOSWindow && theOSWindow->fOwner)
		{
		Point currentPoint;
		::GetDragMouse(inDragReference, &currentPoint, nil);
		ZPoint windowLocation = sFromGlobal(inWindowRef, currentPoint);
		ZOSWindow_Mac::Drag theDrag(inDragReference);
		switch (inMessage)
			{
			case kDragTrackingEnterWindow:
				{
				ZAssertStop(kDebug_Mac, fDragActive == false);

				fDragActive = true;
				ZLocker locker(theOSWindow->GetLock());
#if ZCONFIG(Processor, PPC)
				if (::sGestalt_WindowManager())
					::HiliteWindowFrameForDrag(inWindowRef, true);
#endif // ZCONFIG(Processor, PPC)
				theOSWindow->fOwner->OwnerDrag(windowLocation, ZMouse::eMotionEnter, theDrag);
				fLocation_LastDragDrop = windowLocation;
				break;
				}			
			case kDragTrackingInWindow:
				{
				ZAssertStop(kDebug_Mac, fDragActive == true);

				ZLocker locker(theOSWindow->GetLock());
				ZPoint windowLocation = sFromGlobal(inWindowRef, currentPoint);
				if (fLocation_LastDragDrop == windowLocation)
					theOSWindow->fOwner->OwnerDrag(windowLocation, ZMouse::eMotionLinger, theDrag);
				else
					theOSWindow->fOwner->OwnerDrag(windowLocation, ZMouse::eMotionMove, theDrag);
				fLocation_LastDragDrop = windowLocation;
				break;
				}
			case kDragTrackingLeaveWindow:
				{
				fLocation_LastDragDrop = windowLocation;
				if (!fDropOccurred)
					{
					ZLocker locker(theOSWindow->GetLock());
					theOSWindow->fOwner->OwnerDrag(windowLocation, ZMouse::eMotionLeave, theDrag);
					}
				fDropOccurred = false;
				fDragActive = false;
#if ZCONFIG(Processor, PPC)
				if (::sGestalt_WindowManager())
					::HiliteWindowFrameForDrag(inWindowRef, false);
#endif // ZCONFIG(Processor, PPC)
				break;
				}
			}
		}
//	ZThreadTM_Yield();
	return noErr; //dragNotAcceptedErr;
	}

DEFINE_API(OSErr) ZOSApp_Mac::sDragReceiveHandler(WindowRef inWindowRef, void* inRefCon, DragReference inDragReference)
	{ return ZOSApp_Mac::sGet()->DragReceiveHandler(inWindowRef, inRefCon, inDragReference); }

OSErr ZOSApp_Mac::DragReceiveHandler(WindowRef inWindowRef, void* inRefCon, DragReference inDragReference)
	{
	ZOSWindow_Mac* theOSWindow = ZOSWindow_Mac::sFromWindowRef(inWindowRef);
	if (theOSWindow && theOSWindow->fOwner)
		{
		Point currentPoint;
		::GetDragMouse(inDragReference, &currentPoint, nil);
		ZPoint windowLocation = sFromGlobal(inWindowRef, currentPoint);
		ZLocker locker(theOSWindow->GetLock());
		ZOSWindow_Mac::Drag theDrag(inDragReference);
		theOSWindow->fOwner->OwnerDrag(windowLocation, ZMouse::eMotionLeave, theDrag);

		ZOSWindow_Mac::Drop theDrop(inDragReference);
		theOSWindow->fOwner->OwnerDrop(windowLocation, theDrop);
		fDropOccurred = true;
		}
	return noErr; //dragNotAcceptedErr;
	}

// ==================================================

void ZOSApp_Mac::IdleThread()
	{
	while (!fSemaphore_IdleExited)
		{
		try
			{
			ZMutexLocker structureLocker(fMutex_Structure);
			for (WindowRef iterWindow = ::GetWindowList(); iterWindow; iterWindow = MacGetNextWindow(iterWindow))
				{
				if (ZOSWindow_Mac* currentOSWindow = ZOSWindow_Mac::sFromWindowRef(iterWindow))
					currentOSWindow->Internal_RecordIdle();
				}

			this->Internal_RecordIdle();
			}
		catch (...)
			{}

		ZThread::sSleep(20);
		}
	fSemaphore_IdleExited->Signal(1);
	}

void ZOSApp_Mac::sIdleThread(ZOSApp_Mac* inOSApp)
	{ inOSApp->IdleThread(); }

#endif // ZCONFIG(API_OSWindow, Mac)

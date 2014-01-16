static const char rcsid[] = "@(#) $Id: ZOSWindow_Carbon.cpp,v 1.36 2006/10/13 20:34:03 agreen Exp $";

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

#include "ZOSWindow_Carbon.h"

#if ZCONFIG(API_OSWindow, Carbon)

#include "ZDC_QD.h"
#include "ZDragClip.h"
#include "ZFile_MacClassic.h"
#include "ZLog.h"
#include "ZMacOSX.h"
#include "ZMain.h"
#include "ZMemory.h"
#include "ZMenu_Carbon.h"
#include "ZString.h"
#include "ZTicks.h"
#include "ZUtil_Mac_HL.h"
#include "ZUtil_Mac_LL.h"
#include "ZUtil_MacOSX.h"
#include "ZUtil_STL.h"
#include "ZUtil_Tuple.h"

#if ZCONFIG(OS, MacOSX)
#	include <HIToolbox/HIView.h>
#endif

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// =================================================================================================
#pragma mark -
#pragma mark * Fixups

#if UNIVERSAL_INTERFACES_VERSION <= 0x0341
// Taken from Universal Headers 3.4.2
enum {

  /*
   * This event parameter may be added to any event that is posted to
   * the main event queue. When the event is removed from the queue and
   * sent to the event dispatcher, the dispatcher will retrieve the
   * EventTargetRef contained in this parameter and send the event
   * directly to that event target. If this parameter is not available
   * in the event, the dispatcher will send the event to a suitable
   * target, or to the application target if no more specific target is
   * appropriate. Available in CarbonLib 1.3.1 and later, and Mac OS X.
   */
  kEventParamPostTarget         = FOUR_CHAR_CODE('ptrg'), /* typeEventTargetRef*/

  /*
   * Indicates an event parameter of type EventTargetRef.
   */
  typeEventTargetRef            = FOUR_CHAR_CODE('etrg') /* EventTargetRef*/
};
#endif

// =================================================================================================
#pragma mark -
#pragma mark * kDebug_Carbon

#define kDebug_Carbon 1

// =================================================================================================
#pragma mark -
#pragma mark * Event helpers

static const UInt32 kEventClassID_ZooLib = 'ZooL';
static const UInt32 kEventKindID_Message = 'Mess';
static const UInt32 kEventKindID_RAEL = 'RAEL';

static bool sGetParam(EventRef iEvent, EventParamName iName, EventParamType iType, size_t iBufferSize, void* iBuffer)
	{
	return noErr == ::GetEventParameter(iEvent, iName, iType, nil, iBufferSize, nil, iBuffer);
	}
  
template <class T>
bool sGetParam_T(EventRef iEventRef, EventParamName iName, EventParamType iType, T& oT)
	{
	return sGetParam(iEventRef, iName, iType, sizeof(oT), &oT);
	}

template <class T>
T sGetParam_T(EventRef iEventRef, EventParamName iName, EventParamType iType)
	{
	T theT = T();
	sGetParam(iEventRef, iName, iType, sizeof(theT), &theT);
	return theT;
	}

static size_t sGetParamLength(EventRef iEventRef, EventParamName iName, EventParamType iType)
	{
	UInt32 theLength;
	if (noErr == ::GetEventParameter(iEventRef, iName, iType, nil, 0, &theLength, nil))
		return theLength;
	return 0;
	}

static const PropertyCreator sPropertyCreator_ZooLib = 'ZooL';
static const PropertyTag sPropertyTag_OSWindow = 'OSWi';

static ZOSWindow_Carbon* sFromWindowRef(WindowRef iWindowRef)
	{
	if (iWindowRef)
		{
		ZOSWindow_Carbon* result = nil;
		if (noErr == ::GetWindowProperty(iWindowRef,
			sPropertyCreator_ZooLib, sPropertyTag_OSWindow,
			sizeof(result), nil, &result))
			{
			return result;
			}
		}

	return nil;
	}

static ZPoint sStructureToContent(WindowRef iWindowRef, ZPoint iStructure)
	{
	Rect contentBounds;
	::GetWindowBounds(iWindowRef, kWindowGlobalPortRgn, &contentBounds);

	Rect structureBounds;
	::GetWindowBounds(iWindowRef, kWindowStructureRgn, &structureBounds);

	iStructure.h -= contentBounds.left - structureBounds.left;
	iStructure.v -= contentBounds.top - structureBounds.top;

	return iStructure;
	}

static ZPoint sGlobalToContent(WindowRef iWindowRef, ZPoint iGlobal)
	{
	Rect contentBounds;
	::GetWindowBounds(iWindowRef, kWindowGlobalPortRgn, &contentBounds);

	iGlobal.h -= contentBounds.left;
	iGlobal.v -= contentBounds.top;

	return iGlobal;
	}

static string sStringFromUInt32(uint32 iVal)
	{
	string result;
	result += char(iVal >> 24);
	result += char(iVal >> 16);
	result += char(iVal >> 8);
	result += char(iVal);
	return result;
	}

static ZMessage sMouseEventToMessage(EventRef iEvent)
	{
	ZMessage theMessage;
	
	theMessage.SetPointer("EventRef", iEvent);

	theMessage.SetString("eventClass", sStringFromUInt32(::GetEventClass(iEvent)));
	theMessage.SetInt32("eventKind", ::GetEventKind(iEvent));

	HIPoint globalLocation = sGetParam_T<HIPoint>(iEvent, kEventParamMouseLocation, typeHIPoint);
	theMessage.SetPoint("kEventParamMouseLocation", ZPoint(globalLocation.x, globalLocation.y));

#if ZCONFIG(OS, MacOSX)
	HIPoint windowLocation = sGetParam_T<HIPoint>(iEvent, kEventParamWindowMouseLocation, typeHIPoint);
	theMessage.SetPoint("kEventParamWindowMouseLocation", ZPoint(windowLocation.x, windowLocation.y));

	WindowPartCode theWPC = sGetParam_T<WindowPartCode>(iEvent, kEventParamWindowPartCode, typeWindowPartCode);
	theMessage.SetInt32("kEventParamWindowPartCode", theWPC);
#endif // ZCONFIG(OS, MacOSX)

	WindowRef theWindowRef = sGetParam_T<WindowRef>(iEvent, kEventParamWindowRef, typeWindowRef);
	theMessage.SetPointer("kEventParamWindowRef", theWindowRef);

	UInt32 mouseChord = sGetParam_T<UInt32>(iEvent, kEventParamMouseChord, typeUInt32);
	theMessage.SetInt32("kEventParamMouseChord", mouseChord);


	EventTime theEventTime = ::GetEventTime(iEvent);
	theMessage.SetInt64("when", theEventTime * 1000000);

	UInt32 modifierKeys; 
	sGetParam(iEvent, kEventParamKeyModifiers, typeUInt32, sizeof(modifierKeys), &modifierKeys); 
	theMessage.SetBool("command", modifierKeys & cmdKey);
	theMessage.SetBool("option", modifierKeys & optionKey);
	theMessage.SetBool("shift", modifierKeys & shiftKey);
	theMessage.SetBool("control", modifierKeys & controlKey);
	theMessage.SetBool("capsLock", modifierKeys & alphaLock);

	return theMessage;
	}

static ZMessage sMouseEventToMessage(WindowRef iWindowRef, EventRef iEvent)
	{
	ZMessage theMessage = sMouseEventToMessage(iEvent);

	HIPoint globalLocation = sGetParam_T<HIPoint>(iEvent, kEventParamMouseLocation, typeHIPoint);
	ZPoint contentLocation = sGlobalToContent(iWindowRef, ZPoint(globalLocation.x, globalLocation.y));
	theMessage.SetPoint("where", contentLocation);

	WindowRef theWindowRef = sGetParam_T<WindowRef>(iEvent, kEventParamWindowRef, typeWindowRef);
	theMessage.SetBool("contained", theWindowRef == iWindowRef);
	return theMessage;
	}

#if ZCONFIG(OS, MacOSX)
#include <sys/stat.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

static void* TkMacOSXGetNamedSymbol(const char* module, const char* symbol)
	{
	NSSymbol nsSymbol = NULL;
	if (module && *module)
		{
		if (NSIsSymbolNameDefinedWithHint(symbol, module))
			nsSymbol = NSLookupAndBindSymbolWithHint(symbol, module);
		}
	else
		{
		if (NSIsSymbolNameDefined(symbol))
			nsSymbol = NSLookupAndBindSymbol(symbol);
		}

	if (nsSymbol)
		return NSAddressOfSymbol(nsSymbol);

	return NULL;
	}

static void sInitTraceEvents()
	{
	void (*TraceEventByName)(char*) = reinterpret_cast<void(*)(char*)>(TkMacOSXGetNamedSymbol("HIToolbox", "_TraceEventByName"));
    if (TraceEventByName)
    	{
		/* Carbon-internal event debugging (c.f. Technote 2124) */
		TraceEventByName("kEventMouseDown");
		TraceEventByName("kEventMouseUp");
		TraceEventByName("kEventMouseWheelMoved");
		TraceEventByName("kEventMouseScroll");
		TraceEventByName("kEventWindowUpdate");
		TraceEventByName("kEventWindowDrawContent");
		TraceEventByName("kEventWindowActivated");
		TraceEventByName("kEventWindowDeactivated");
		TraceEventByName("kEventRawKeyDown");
		TraceEventByName("kEventRawKeyRepeat");
		TraceEventByName("kEventRawKeyUp");
		TraceEventByName("kEventRawKeyModifiersChanged");
		TraceEventByName("kEventRawKeyRepeat");
		TraceEventByName("kEventAppActivated");
		TraceEventByName("kEventAppDeactivated");
		TraceEventByName("kEventAppQuit");
		TraceEventByName("kEventMenuBeginTracking");
		TraceEventByName("kEventMenuEndTracking");
		TraceEventByName("kEventCommandProcess");
		TraceEventByName("kEventCommandUpdateStatus");
		TraceEventByName("kEventWindowExpanded");
		TraceEventByName("kEventAppHidden");
		TraceEventByName("kEventAppShown");
		TraceEventByName("kEventAppAvailableWindowBoundsChanged");

		TraceEventByName("kEventTextInputUnicodeForKeyEvent");

		TraceEventByName("kEventMenuBeginTracking");
		TraceEventByName("kEventMenuEndTracking");
		TraceEventByName("kEventMenuMatchKey");
		TraceEventByName("kEventMenuEnableItems");
		TraceEventByName("kEventMenuPopulate");
	    }
	   }
#endif // ZCONFIG(OS, MacOSX)

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_CarbonWindow

class ZDCCanvas_QD_CarbonWindow : public ZDCCanvas_QD
	{
public:
	ZDCCanvas_QD_CarbonWindow(ZOSWindow_Carbon* inWindow, CGrafPtr inGrafPtr, ZMutex* inMutexToCheck, ZMutex* inMutexToLock);
	virtual ~ZDCCanvas_QD_CarbonWindow();

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
	ZOSWindow_Carbon* fOSWindow;
	};

ZDCCanvas_QD_CarbonWindow::ZDCCanvas_QD_CarbonWindow(ZOSWindow_Carbon* inWindow, CGrafPtr inGrafPtr, ZMutex* inMutexToCheck, ZMutex* inMutexToLock)
	{
	ZMutexLocker locker(sMutex_List);
	fOSWindow = inWindow;
	fMutexToCheck = inMutexToCheck;
	fMutexToLock = inMutexToLock;
	this->Internal_Link(inGrafPtr);
	}

ZDCCanvas_QD_CarbonWindow::~ZDCCanvas_QD_CarbonWindow()
	{
	ZAssertLocked(kDebug_Carbon, sMutex_List);
	fGrafPtr = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	this->Internal_Unlink();
	}

void ZDCCanvas_QD_CarbonWindow::OSWindowDisposed()
	{
	fOSWindow = nil;
	fGrafPtr = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	}

void ZDCCanvas_QD_CarbonWindow::Finalize()
	{
	ZMutexLocker locker(sMutex_List);

	if (fOSWindow)
		{
		fOSWindow->FinalizeCanvas(this);
		}
	else
		{
		// If we don't have a window, fMutexToCheck and fMutexToLock must already be nil
		ZAssertStop(kDebug_Carbon, fMutexToCheck == nil && fMutexToLock == nil);
		this->FinalizationComplete();
		delete this;
		}
	}

void ZDCCanvas_QD_CarbonWindow::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fGrafPtr)
		return;

	ZAssertStop(kDebug_Carbon, fMutexToCheck == nil || fMutexToCheck->IsLocked());

	ZMutexLocker locker(*fMutexToLock);

#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	// Mark our clip as having been changed, because it's going to be physically changed to do the scroll
	// or we're going to call Internal_RecordInvalidation, which will also invalidate our clip.
	++fChangeCount_Clip;

	// We don't bother ensuring that our grafPort's origin is at (0,0), we just compensate for whatever
	// value it has by adding in currentQDOrigin as needed.
	Rect portBounds;
	::GetPortBounds(fGrafPtr, &portBounds);
	ZPoint currentQDOrigin(portBounds.left, portBounds.top);
	ZPoint offset(hDelta, vDelta);

	// Work out effectiveVisRgn, the set of pixels we're able/allowed to touch. It's made
	// up of our port's real visRgn, intersected with inRect, and with our window's exclude rgn removed
	ZDCRgn effectiveVisRgn;
	::GetPortVisibleRegion(fGrafPtr, effectiveVisRgn.GetRgnHandle());
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
	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// And set the clip (drawnRgn)
	::SetClip((drawnRgn + currentQDOrigin).GetRgnHandle());

	Rect qdSourceRect = drawnRgn.Bounds() + currentQDOrigin - offset;
	Rect qdDestRect = drawnRgn.Bounds() + currentQDOrigin;
	::CopyBits(GetPortBitMapForCopyBits(fGrafPtr), GetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);

	// Finally, let our window know what additional pixels are invalid
	fOSWindow->InvalidateRegion(invalidRgn);
	}

void ZDCCanvas_QD_CarbonWindow::CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
	ZRef<ZDCCanvas_QD> sourceCanvasQD = ZRefDynamicCast<ZDCCanvas_QD>(inSourceCanvas);
	ZAssertStop(kDebug_Carbon, sourceCanvasQD);

	GrafPtr sourceGrafPtr = (GrafPtr)sourceCanvasQD->Internal_GetGrafPtr();
	if (fGrafPtr == nil || sourceGrafPtr == nil)
		return;

	ZAssertStop(kDebug_Carbon, fMutexToCheck == nil || fMutexToCheck->IsLocked());

	ZMutexLocker locker(*fMutexToLock);

#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	++fChangeCount_Clip;

	ZRect localDestRect = inSourceRect + (ioState.fOrigin + inDestLocation - inSourceRect.TopLeft());
	Rect portBounds;
	::GetPortBounds(fGrafPtr, &portBounds);
	ZPoint currentDestQDOrigin(portBounds.left, portBounds.top);

	ZDCRgn effectiveDestVisRgn;
	::GetPortVisibleRegion(fGrafPtr, effectiveDestVisRgn.GetRgnHandle());
	effectiveDestVisRgn -= currentDestQDOrigin;
	effectiveDestVisRgn &= localDestRect;
	effectiveDestVisRgn -= fOSWindow->Internal_GetExcludeRgn();

	ZDCRgn destRgn = (ioState.fClip + ioState.fClipOrigin) & effectiveDestVisRgn;

	ZRect localSourceRect = inSourceRect + inSourceState.fOrigin;
	::GetPortBounds(sourceGrafPtr, &portBounds);
	ZPoint currentSourceQDOrigin(portBounds.left, portBounds.top);

	ZDCRgn effectiveSourceVisRgnTranslatedToDestCoords;
	::GetPortVisibleRegion(sourceGrafPtr, effectiveSourceVisRgnTranslatedToDestCoords.GetRgnHandle());
	effectiveSourceVisRgnTranslatedToDestCoords -= currentSourceQDOrigin;
	effectiveSourceVisRgnTranslatedToDestCoords &= localSourceRect;
	effectiveSourceVisRgnTranslatedToDestCoords -= sourceCanvasQD->Internal_GetExcludeRgn();
	effectiveSourceVisRgnTranslatedToDestCoords += localDestRect.TopLeft() - localSourceRect.TopLeft();

	ZDCRgn drawnRgn = destRgn & effectiveSourceVisRgnTranslatedToDestCoords;

	ZDCRgn invalidRgn = destRgn - drawnRgn;

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// And set the clip (drawnRgn)
	::SetClip((drawnRgn + currentDestQDOrigin).GetRgnHandle());

	Rect qdSourceRect = drawnRgn.Bounds() + (currentSourceQDOrigin + localSourceRect.TopLeft() - localDestRect.TopLeft());
	Rect qdDestRect = drawnRgn.Bounds() + currentDestQDOrigin;
	::CopyBits(GetPortBitMapForCopyBits(sourceGrafPtr), GetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);

	// Finally, let our window know what additional pixels are invalid
	fOSWindow->InvalidateRegion(invalidRgn);
	}

ZRef<ZDCCanvas> ZDCCanvas_QD_CarbonWindow::CreateOffScreen(const ZRect& inCanvasRect)
	{
	if (!fGrafPtr)
		return ZRef<ZDCCanvas>();

	ZAssertStop(kDebug_Carbon, fMutexToCheck == nil || fMutexToCheck->IsLocked());

#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
#endif

	::SetGWorld(fGrafPtr, nil);

	Point zeroPoint;
	Rect portBounds;
	::GetPortBounds(fGrafPtr, &portBounds);
	zeroPoint.h = portBounds.left;
	zeroPoint.v = portBounds.top;
	::LocalToGlobal(&zeroPoint);

	return new ZDCCanvas_QD_OffScreen(inCanvasRect + zeroPoint);
	}

ZDCRgn ZDCCanvas_QD_CarbonWindow::Internal_CalcClipRgn(const ZDCState& inState)
	{
	return (inState.fClip + inState.fClipOrigin) - fOSWindow->Internal_GetExcludeRgn();
	}

ZDCRgn ZDCCanvas_QD_CarbonWindow::Internal_GetExcludeRgn()
	{ return fOSWindow->Internal_GetExcludeRgn(); }

void ZDCCanvas_QD_CarbonWindow::InvalClip()
	{
	ZAssertStop(kDebug_Carbon, fMutexToLock->IsLocked());
	++fChangeCount_Clip;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Carbon

WindowRef ZOSWindow_Carbon::sCreateWindowRef(const ZOSWindow::CreationAttributes& iAttributes)
	{
	ZOSWindow::CreationAttributes localAttributes = iAttributes;
	WindowClass theWC = kDocumentWindowClass;
	WindowAttributes theWA = 0;
	WindowGroupRef newWindowGroupRef = nil;
	ZOSWindow::Layer defaultLayer;
	WindowActivationScope theWAS = -1;
	switch (localAttributes.fLook)
		{
		case lookDocument:
			defaultLayer = layerDocument;
			theWC = kDocumentWindowClass;
			theWA |= kWindowCollapseBoxAttribute;
			break;
		case lookPalette:
			defaultLayer = layerFloater;
			theWC = kFloatingWindowClass;
			theWA |= kWindowCollapseBoxAttribute;
			break;
		case lookModal:
			defaultLayer = layerDialog;
			theWC = kMovableModalWindowClass;
			localAttributes.fHasCloseBox = false;
			break;
		case lookMovableModal:
			defaultLayer = layerDialog;
			theWC = kMovableModalWindowClass;
			localAttributes.fHasCloseBox = false;
			break;
		case lookAlert:
			defaultLayer = layerDialog;
			theWC = kAlertWindowClass;
			localAttributes.fHasCloseBox = false;
			break;
		case lookMovableAlert:
			defaultLayer = layerDialog;
			theWC = kMovableAlertWindowClass;
			localAttributes.fHasCloseBox = false;
			break;
		case lookThinBorder:
			defaultLayer = layerDocument;
			theWC = kPlainWindowClass;
			localAttributes.fHasCloseBox = false;
			localAttributes.fHasSizeBox = false;
			localAttributes.fHasZoomBox = false;
			break;
		case lookMenu:
			defaultLayer = layerHelp;
			theWC = kHelpWindowClass;
			localAttributes.fHasCloseBox = false;
			localAttributes.fHasSizeBox = false;
			localAttributes.fHasZoomBox = false;
			break;
		case lookHelp:
			defaultLayer = layerHelp;
			theWC = kHelpWindowClass;
			localAttributes.fHasCloseBox = false;
			localAttributes.fHasSizeBox = false;
			localAttributes.fHasZoomBox = false;
			break;
		}

	if (localAttributes.fLayer != defaultLayer)
		{
		switch (localAttributes.fLayer)
			{
			case layerSinker:
			case layerDocument:
				newWindowGroupRef = GetWindowGroupOfClass(kDocumentWindowClass);
				break;
				
			case layerFloater:
				newWindowGroupRef = GetWindowGroupOfClass(kFloatingWindowClass);
				theWAS = kWindowActivationScopeNone;
				theWA |= kWindowHideOnSuspendAttribute;
				break;

			case layerDialog:
				newWindowGroupRef = GetWindowGroupOfClass(kMovableModalWindowClass);
				break;

			case layerMenu:
				newWindowGroupRef = GetWindowGroupOfClass(kHelpWindowClass);
				theWAS = kWindowActivationScopeNone;
				break;

			case layerHelp:
				newWindowGroupRef = GetWindowGroupOfClass(kHelpWindowClass);
				theWAS = kWindowActivationScopeNone;
				break;
			}
		}

	if (localAttributes.fHasSizeBox)
		theWA |= kWindowResizableAttribute;

	if (localAttributes.fHasCloseBox)
		theWA |= kWindowCloseBoxAttribute;

	if (localAttributes.fHasZoomBox)
		theWA |= kWindowFullZoomAttribute;

	theWA |= kWindowStandardHandlerAttribute;
	theWA |= kWindowLiveResizeAttribute;
	
	#if ZCONFIG(OS, MacOSX)
		// To use CoreGraphics we must have a compositing window. It's still legal to use QD
		// with a compositing window, although QD blows away alpha.
		theWA |= kWindowCompositingAttribute;
		theWA |= kWindowAsyncDragAttribute;
	#endif

	Rect theBounds = localAttributes.fFrame;

	OSErr err;
	
	WindowRef theWindowRef;
	err = ::CreateNewWindow(theWC, theWA, &theBounds, &theWindowRef);
	if (err != noErr)
		throw runtime_error(ZString::sFormat("CreateNewWindow error: %d", err));

	if (newWindowGroupRef)
		{
		err = ::SetWindowGroup(theWindowRef, newWindowGroupRef);
		if (err != noErr)
			throw runtime_error(ZString::sFormat("SetWindowGroup error: %d", err));
		}

	if (theWAS != -1)
		{
		err = ::SetWindowActivationScope(theWindowRef, theWAS);
		if (err != noErr)
			throw runtime_error(ZString::sFormat("SetWindowActivationScope error: %d", err));
		}

	return theWindowRef;
	}

ZOSWindow_Carbon::ZOSWindow_Carbon(WindowRef iWindowRef)
:	fMutex("ZOSWindow_Carbon::fMutex", true),
	fWindowRef(iWindowRef),
	fEventTargetRef_Window(nil),
	fEventHandlerRef_Window(nil),
	fEventHandlerRef_Content(nil),
	fEventLoopTimerRef_Idle(nil),
	fCanvas(nil),
	fCCrsrHandle(nil)
	{
	fEventTargetRef_Window = ::GetWindowEventTarget(fWindowRef);

	ZOSWindow_Carbon* thisPtr = this;
	::SetWindowProperty(fWindowRef, sPropertyCreator_ZooLib, sPropertyTag_OSWindow, sizeof(thisPtr), &thisPtr);

	::SetThemeWindowBackground(fWindowRef, kThemeBrushSheetBackgroundTransparent, true);

	static const EventTypeSpec sEvents_Window[] =
		{
		{ kEventClassWindow, kEventWindowShown },
		{ kEventClassWindow, kEventWindowHidden },
		{ kEventClassWindow, kEventWindowClose },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassWindow, kEventWindowBoundsChanging },		
//		{ kEventClassWindow, kEventWindowGetIdealSize },
		{ kEventClassWindow, kEventWindowCursorChange },

		{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },

		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },

		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, 11 }, // MightyMouse wheels
		{ kEventClassMouse, kEventMouseWheelMoved },
		#if ZCONFIG(OS, MacOSX)
			{ kEventClassMouse, kEventMouseEntered },
			{ kEventClassMouse, kEventMouseExited },
		#endif

		{ kEventClassID_ZooLib, kEventKindID_Message }
		};

	::InstallEventHandler(fEventTargetRef_Window, sEventHandlerUPP_Window,
		countof(sEvents_Window), sEvents_Window,
		this, &fEventHandlerRef_Window);


	static const EventTypeSpec sEvents_WindowUpdate[] =
		{
		{ kEventClassWindow, kEventWindowUpdate },
		{ kEventClassWindow, kEventWindowDrawContent }
		};

	#if ZCONFIG(OS, MacOSX)
		WindowAttributes theWA;
		::GetWindowAttributes(fWindowRef, &theWA);
		if (theWA & kWindowCompositingAttribute)
			fCompositing = true;
		else
			fCompositing = false;

		if (fCompositing)
			{
			// We have compositing window, so we'll do drawing through the content HIView
			HIViewRef contentViewRef;
			::HIViewFindByID(::HIViewGetRoot(fWindowRef), kHIViewWindowContentID, &contentViewRef);
		
			static EventTypeSpec sEvents_Content[] =
				{
				{ kEventClassControl, kEventControlDraw },
				};
		
			::InstallEventHandler(::GetControlEventTarget(contentViewRef), sEventHandlerUPP_Content,
				countof(sEvents_Content), sEvents_Content,
				this, &fEventHandlerRef_Content);
			}
		else
	#endif
		{
		::AddEventTypesToHandler(
			fEventHandlerRef_Window, countof(sEvents_WindowUpdate), sEvents_WindowUpdate);
		}

	::InstallEventLoopTimer(::GetCurrentEventLoop(), 0.1, 0.1,
		sEventLoopTimerUPP_Idle, this, &fEventLoopTimerRef_Idle);

	this->InvalidateRegion(ZRect(this->GetSize()));
	}

ZOSWindow_Carbon::~ZOSWindow_Carbon()
	{
	if (ZRef<ZDCCanvas_QD_CarbonWindow> tempCanvas = fCanvas)
		{
		fCanvas = nil;
		tempCanvas->OSWindowDisposed();
		}

	if (fEventLoopTimerRef_Idle)
		::RemoveEventLoopTimer(fEventLoopTimerRef_Idle);
	fEventLoopTimerRef_Idle = nil;

	if (fEventHandlerRef_Window)
		::RemoveEventHandler(fEventHandlerRef_Window);
	fEventHandlerRef_Window = nil;

	if (fEventHandlerRef_Content)
		::RemoveEventHandler(fEventHandlerRef_Content);
	fEventHandlerRef_Content = nil;

	fEventTargetRef_Window = nil;

	if (fWindowRef)
		::ReleaseWindow(fWindowRef);
	fWindowRef = nil;

	if (fCCrsrHandle)
		::DisposeCCursor(fCCrsrHandle);

	ZOSApp_Carbon::sGet()->Disposed(this);
	}

ZMutexBase& ZOSWindow_Carbon::GetLock()
	{ return fMutex; }

void ZOSWindow_Carbon::QueueMessage(const ZMessage& iMessage)
	{
	ZMessage* theMessage = new ZMessage(iMessage);

	EventRef theEventRef;
	::MacCreateEvent(nil, kEventClassID_ZooLib, kEventKindID_Message,
		::GetCurrentEventTime(), kEventAttributeNone, &theEventRef);
	::SetEventParameter(theEventRef, 'Mess', typePtr, sizeof(theMessage), &theMessage);
	::SetEventParameter(theEventRef, kEventParamPostTarget, typeEventTargetRef, sizeof(fEventTargetRef_Window), &fEventTargetRef_Window);
	::PostEventToQueue(::GetMainEventQueue(), theEventRef, kEventPriorityStandard);
	::ReleaseEvent(theEventRef);
	}

bool ZOSWindow_Carbon::DispatchMessage(const ZMessage& iMessage)
	{
	return ZOSWindow::DispatchMessage(iMessage);
	}

void ZOSWindow_Carbon::SetShownState(ZShownState iState)
	{
	if (iState == eZShownStateShown)
		::ShowWindow(fWindowRef);
	else
		::HideWindow(fWindowRef);
	}

ZShownState ZOSWindow_Carbon::GetShownState()
	{
	if (::IsWindowVisible(fWindowRef))
		return eZShownStateShown;
	else
		return eZShownStateHidden;
	}

void ZOSWindow_Carbon::SetTitle(const string& iTitle)
	{
	CFStringRef theCFS = ZUtil_MacOSX::sCreateCFStringUTF8(iTitle);
	::SetWindowTitleWithCFString(fWindowRef, theCFS);
	::CFRelease(theCFS);
	}

string ZOSWindow_Carbon::GetTitle()
	{
	string result;
	CFStringRef theCFS;
	if (noErr == ::CopyWindowTitleAsCFString(fWindowRef, &theCFS))
		{
		result = ZUtil_MacOSX::sAsUTF8(theCFS);
		::CFRelease(theCFS);
		}
	return result;
	}

void ZOSWindow_Carbon::SetCursor(const ZCursor& iCursor)
	{
//	ZAssertLocked(kDebug_Carbon, fMutex_Structure);

	if (iCursor.IsCustom())
		{
		ZPoint theHotSpot;
		ZDCPixmap colorPixmap, monoPixmap, maskPixmap;
		iCursor.Get(colorPixmap, monoPixmap, maskPixmap, theHotSpot);

		// We insist on having a mask and either a color or a mono cursor.
		ZAssertStop(kDebug_Carbon, maskPixmap && (colorPixmap || monoPixmap));
		if (!colorPixmap)
			colorPixmap = monoPixmap;

		CCrsrHandle theCCrsrHandle = nil;
		if (colorPixmap)
			{
			theCCrsrHandle = CCrsrHandle(::NewHandle(sizeof(CCrsr)));
			ZUtil_Mac_LL::HandleLocker locker1((Handle)theCCrsrHandle);
			theCCrsrHandle[0]->crsrType = 0x8001;
			theCCrsrHandle[0]->crsrMap = (PixMapHandle)::NewHandle(sizeof(PixMap));
			ZUtil_Mac_LL::HandleLocker locker2(Handle(theCCrsrHandle[0]->crsrMap));
			ZUtil_Mac_LL::sSetupPixMapColor(ZPoint(16, 16), 16, theCCrsrHandle[0]->crsrMap[0][0]);
			theCCrsrHandle[0]->crsrData = ::NewHandleClear(2 * 16 * 16);
			ZUtil_Mac_LL::HandleLocker locker3(Handle(theCCrsrHandle[0]->crsrData));

			ZDCPixmapNS::RasterDesc theRasterDesc;
			theRasterDesc.fPixvalDesc.fDepth = 16;
			theRasterDesc.fPixvalDesc.fBigEndian = true;
			theRasterDesc.fRowBytes = 2 * 16;
			theRasterDesc.fRowCount = 16;
			theRasterDesc.fFlipped = false;

			colorPixmap.CopyTo(ZPoint(0, 0),
								theCCrsrHandle[0]->crsrData[0],
								theRasterDesc,
								ZDCPixmapNS::PixelDesc(0x7C00, 0x03E0, 0x001F, 0),
								ZRect(16, 16));

			theCCrsrHandle[0]->crsrXData = ::NewHandle(2);
			theCCrsrHandle[0]->crsrXValid = 0;
			theCCrsrHandle[0]->crsrXHandle = nil;
			theCCrsrHandle[0]->crsrHotSpot = theHotSpot;
			theCCrsrHandle[0]->crsrXTable = 0;
			theCCrsrHandle[0]->crsrID = ::GetCTSeed();

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
				monoPixmap.CopyTo(ZPoint(0, 0), &theCCrsrHandle[0]->crsr1Data, theRasterDesc, thePixelDesc, ZRect(16, 16));
				}
			else
				{
				// Have to use a mono-ized version of the color pixmap.
				colorPixmap.CopyTo(ZPoint(0, 0), &theCCrsrHandle[0]->crsr1Data, theRasterDesc, thePixelDesc, ZRect(16, 16));
				}
			// Invert it, to match Mac requirements
			for (size_t x = 0; x < 16; ++x)
				theCCrsrHandle[0]->crsr1Data[x] ^= 0xFFFF;

			// Move the mask over
			maskPixmap.CopyTo(ZPoint(0, 0), &theCCrsrHandle[0]->crsrMask, theRasterDesc, thePixelDesc, ZRect(16, 16));

			::SetCCursor(theCCrsrHandle);
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

			// Invert the cursor data, to match Mac requirements
			for (size_t x = 0; x < 16; ++x)
				theMacCursor.data[x] ^= 0xFFFF;

			maskPixmap.CopyTo(ZPoint(0, 0), &theMacCursor.mask, theRasterDesc, thePixelDesc, ZRect(16, 16));

			theMacCursor.hotSpot = theHotSpot;
			::SetCursor(&theMacCursor);
			}
		if (fCCrsrHandle)
			::DisposeCCursor(fCCrsrHandle);
		fCCrsrHandle = theCCrsrHandle;
		}
	else
		{
		UInt32 themeCursor = kThemeArrowCursor;
		switch (iCursor.GetCursorType())
			{
			case ZCursor::eCursorTypeArrow: themeCursor = kThemeArrowCursor; break;
			case ZCursor::eCursorTypeIBeam: themeCursor = kThemeIBeamCursor; break;
			case ZCursor::eCursorTypeCross: themeCursor = kThemeCrossCursor; break;
			case ZCursor::eCursorTypePlus: themeCursor = kThemePlusCursor; break;
			case ZCursor::eCursorTypeWatch: themeCursor = kThemeWatchCursor; break;
			}
		::SetThemeCursor(themeCursor);
		}	
	}

void ZOSWindow_Carbon::SetLocation(ZPoint iLocation)
	{
	::MoveWindow(fWindowRef, iLocation.h, iLocation.v, false);
	}

ZPoint ZOSWindow_Carbon::GetLocation()
	{
	Rect globalBounds;
	::GetWindowBounds(fWindowRef, kWindowGlobalPortRgn, &globalBounds);
	return ZPoint(globalBounds.left, globalBounds.top);
	}

void ZOSWindow_Carbon::SetSize(ZPoint newSize)
	{
	::SizeWindow(fWindowRef, newSize.h, newSize.v, true);
	}

ZPoint ZOSWindow_Carbon::GetSize()
	{
	Rect globalBounds;
	::GetWindowBounds(fWindowRef, kWindowGlobalPortRgn, &globalBounds);
	return ZPoint(globalBounds.right - globalBounds.left, globalBounds.bottom - globalBounds.top);
	}

void ZOSWindow_Carbon::SetFrame(const ZRect& iFrame)
	{
	Rect globalBounds = iFrame;
	::SetWindowBounds(fWindowRef, kWindowContentRgn, &globalBounds);
	}

void ZOSWindow_Carbon::SetSizeLimits(ZPoint iMinSize, ZPoint iMaxSize)
	{
	}

void ZOSWindow_Carbon::PoseModal(bool iRunCallerMessageLoopNested, bool* oCallerExists,
	bool* oCalleeExists)
	{
	::RunAppModalLoopForWindow(fWindowRef);
//	ZDebugLog(0);
	}

void ZOSWindow_Carbon::EndPoseModal()
	{
	QuitAppModalLoopForWindow(fWindowRef);
//	ZDebugLog(0);
	}

bool ZOSWindow_Carbon::WaitingForPoseModal()
	{
	ZDebugLog(0);
	return false;
	}

void ZOSWindow_Carbon::BringFront(bool iMakeVisible)
	{
	if (iMakeVisible)
		::ShowWindow(fWindowRef);
	::SelectWindow(fWindowRef);
	if (iMakeVisible)
		{
		ProcessSerialNumber thePSN;
		::MacGetCurrentProcess(&thePSN);
		#if ZCONFIG(OS, MacOSX)
			::SetFrontProcessWithOptions(&thePSN, kSetFrontProcessFrontWindowOnly);
		#else
			::SetFrontProcess(&thePSN);
		#endif
		}
	}

bool ZOSWindow_Carbon::GetActive()
	{
	return ::IsWindowActive(fWindowRef);
	}

void ZOSWindow_Carbon::GetContentInsets(ZPoint& oTopLeftInset, ZPoint& oBottomRightInset)
	{
	Rect contentBounds;
	::GetWindowBounds(fWindowRef, kWindowGlobalPortRgn, &contentBounds);

	Rect structureBounds;
	::GetWindowBounds(fWindowRef, kWindowStructureRgn, &structureBounds);

	oTopLeftInset.h = contentBounds.left - structureBounds.left;
	oTopLeftInset.v = contentBounds.top - structureBounds.top;

	oBottomRightInset.h = contentBounds.right - structureBounds.right;
	oBottomRightInset.v = contentBounds.bottom - structureBounds.bottom;
	}

ZDCRgn ZOSWindow_Carbon::GetVisibleContentRgn()
	{
	return ZDCRgn(this->GetSize());	
	}

ZPoint ZOSWindow_Carbon::ToGlobal(const ZPoint& iWindowPoint)
	{
	return iWindowPoint + this->GetLocation();
	}

void ZOSWindow_Carbon::InvalidateRegion(const ZDCRgn& iBadRgn)
	{
	#if ZCONFIG(OS, MacOSX)
		if (fCompositing)
			{
			if (iBadRgn)
				{
				fInvalidRgn |= iBadRgn;
				HIViewRef contentViewRef;
				::HIViewFindByID(::HIViewGetRoot(fWindowRef), kHIViewWindowContentID, &contentViewRef);
				::HIViewSetNeedsDisplay(contentViewRef, true);
				}
			return;
			}
	#endif

	if (ZDCRgn temp = iBadRgn)
		{
		// Compensate for any offset that may be in place.
		CGrafPtr theWindowPort = GetWindowPort(fWindowRef);
		Rect theBounds;
		GetPortBounds(GetWindowPort(fWindowRef), &theBounds);
		temp += ZPoint(theBounds.left, theBounds.top);

		::InvalWindowRgn(fWindowRef, temp.GetRgnHandle());
		}
	}

void ZOSWindow_Carbon::UpdateNow()
	{
	if (!::IsWindowUpdatePending(fWindowRef))
		return;

	EventRef theEventRef;
	::MacCreateEvent(nil, kEventClassWindow, kEventWindowUpdate,
		::GetCurrentEventTime(), kEventAttributeNone, &theEventRef);
	::SetEventParameter(theEventRef, kEventParamDirectObject, typeWindowRef, sizeof(fWindowRef), &fWindowRef);
	::SendEventToEventTarget(theEventRef, fEventTargetRef_Window);
	::ReleaseEvent(theEventRef);
	}

ZRef<ZDCCanvas> ZOSWindow_Carbon::GetCanvas()
	{
	if (!fCanvas)
		{
		CGrafPtr theWindowPort = GetWindowPort(fWindowRef);
		fCanvas = new ZDCCanvas_QD_CarbonWindow(this, theWindowPort, nil, &fMutex);
		}
	return fCanvas;
	}

void ZOSWindow_Carbon::AcquireCapture()
	{
	ZOSApp_Carbon::sGet()->AcquireCapture(fWindowRef);
	}

void ZOSWindow_Carbon::ReleaseCapture()
	{
	ZOSApp_Carbon::sGet()->ReleaseCapture(fWindowRef);
	}

void ZOSWindow_Carbon::GetNative(void* oNative)
	{
	*static_cast<WindowRef*>(oNative) = fWindowRef;
	}

void ZOSWindow_Carbon::SetMenuBar(const ZMenuBar& iMenuBar)
	{
	fMenuBar = iMenuBar;
	ZOSApp_Carbon::sGet()->MenuBarChanged(this);
	}

int32 ZOSWindow_Carbon::GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone)
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

int32 ZOSWindow_Carbon::GetScreenIndex(const ZRect& inGlobalRect, bool inReturnMainIfNone)
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

int32 ZOSWindow_Carbon::GetMainScreenIndex()
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
	ZDebugStopf(2, ("ZOSWindow_Carbon::GetMainScreenIndex, no main screen found"));
	return -1;
	}

int32 ZOSWindow_Carbon::GetScreenCount()
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

ZRect ZOSWindow_Carbon::GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly)
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

ZDCRgn ZOSWindow_Carbon::GetDesktopRegion(bool inUsableSpaceOnly)
	{
	ZDCRgn theRgn; // Don't initialize from GetGrayRgn -- it'll take ownership of the RgnHandle
	theRgn = ::GetGrayRgn();
	if (!inUsableSpaceOnly)
		{
		// Add back the menu bar space and any rounded corners.
		ZDebugLogf(0, ("ZOSWindow_Carbon::GetDesktopRegion not done yet"));
//		theRgn |= sGetMainScreenRect(false);
		}
	return theRgn;
	}

void ZOSWindow_Carbon::InvalSizeBox()
	{
	WindowAttributes theWA;
	::GetWindowAttributes(fWindowRef, &theWA);
	if (theWA & kWindowResizableAttribute)
		{
		ZDCRgn result;
		::GetWindowRegion(fWindowRef, kWindowGrowRgn, result.GetRgnHandle());
		result -= this->GetLocation();
		this->InvalidateRegion(result);
		}
	}

ZDCRgn ZOSWindow_Carbon::Internal_GetExcludeRgn()
	{
	#if ZCONFIG(OS, MacOSX)
		if (fCompositing)
			return fInvalidRgn;
	#endif

	ZDCRgn result;
	::GetWindowRegion(fWindowRef, kWindowUpdateRgn, result.GetRgnHandle());
	result -= this->GetLocation();
	return result;
	}

void ZOSWindow_Carbon::FinalizeCanvas(ZDCCanvas_QD_CarbonWindow* iCanvas)
	{
//	ZMutexLocker locker(fMutex_Structure);
	ZAssertStop(kDebug_Carbon, iCanvas == fCanvas);

	if (fCanvas->GetRefCount() > 1)
		{
		fCanvas->FinalizationComplete();
		}
	else
		{
		fCanvas = nil;
		iCanvas->FinalizationComplete();
		delete iCanvas;
		}
	}

EventHandlerUPP ZOSWindow_Carbon::sEventHandlerUPP_Window = NewEventHandlerUPP(sEventHandler_Window);

pascal OSStatus ZOSWindow_Carbon::sEventHandler_Window(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{ return static_cast<ZOSWindow_Carbon*>(iRefcon)->EventHandler_Window(iCallRef, iEvent); }

OSStatus ZOSWindow_Carbon::EventHandler_Window(EventHandlerCallRef iCallRef, EventRef iEvent)
	{
	ZMutexLocker locker(fMutex);
	OSStatus err = eventNotHandledErr;

	const UInt32 theEventKind = ::GetEventKind(iEvent);
	switch (::GetEventClass(iEvent))
		{
		case kEventClassID_ZooLib:
			{
			switch (theEventKind)
				{
				case kEventKindID_Message:
					{
					ZMessage* theMessage;
					if (sGetParam(iEvent, 'Mess', typePtr, sizeof(theMessage), &theMessage))
						{
						try
							{
							this->DispatchMessage(*theMessage);
							}
						catch (...)
							{}
						}
					return noErr;
					}
				}
			break;
			}

		case kEventClassWindow:
			{
			switch (theEventKind)
				{
				case kEventWindowShown:
					{
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:ShownState");
					theMessage.SetInt32("shownState", eZShownStateShown);
					this->DispatchMessageToOwner(theMessage);
					break;
					}

				case kEventWindowHidden:
					{
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:ShownState");
					theMessage.SetInt32("shownState", eZShownStateHidden);
					this->DispatchMessageToOwner(theMessage);
					break;
					}

				case kEventWindowClose:
					{
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:Close");
					this->DispatchMessageToOwner(theMessage);
					return noErr;
					}

				case kEventWindowActivated:
					{
					::CallNextEventHandler(iCallRef, iEvent);
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:Activate");
					theMessage.SetBool("active", true);
					this->DispatchMessageToOwner(theMessage);
					break;
					}

				case kEventWindowDeactivated:
					{
					::CallNextEventHandler(iCallRef, iEvent);
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:Activate");
					theMessage.SetBool("active", false);
					this->DispatchMessageToOwner(theMessage);
					break;
					}

				case kEventWindowBoundsChanging:
					{
					this->InvalSizeBox();
					break;
					}

				case kEventWindowBoundsChanged:
					{
					this->InvalSizeBox();
					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:FrameChange");
					this->DispatchMessageToOwner(theMessage);
					break;
					}

				case kEventWindowCursorChange:
					{
					ZMessage theMessage;
					theMessage.SetString("what", "InvalidCursor");
					this->DispatchMessageToOwner(theMessage);
					return noErr;
					}

				case kEventWindowUpdate:
					{
					ZDCRgn theInvalRgn;
					::GetWindowRegion(fWindowRef, kWindowUpdateRgn, theInvalRgn.GetRgnHandle());
					theInvalRgn -= this->GetLocation();

					::BeginUpdate(fWindowRef);
					::EndUpdate(fWindowRef);

					ZDC theDC(this->GetCanvas(), ZPoint::sZero);
					theDC.SetClip(theInvalRgn);
					theDC.SetInk(fOwner->OwnerGetBackInk(this->GetActive()));
					theDC.Fill(theInvalRgn);
					theDC.SetInk(ZDCInk());
					fOwner->OwnerUpdate(theDC);
					return noErr;
					break;
					}

				case kEventWindowDrawContent:
					{
					ZDC theDC(this->GetCanvas(), ZPoint::sZero);
					ZRect myBounds(this->GetSize());
					theDC.SetClip(myBounds);
					theDC.SetInk(ZRGBColor::sGreen);
					theDC.Fill(myBounds);
					theDC.SetInk(ZDCInk());
					fOwner->OwnerUpdate(theDC);
					return noErr;
					}

//				case kEventWindowGetIdealSize:
//					{
//					break;
//					}
				}
			break;
			}


		case kEventClassTextInput:
			{
			switch (theEventKind)
				{
				case kEventTextInputUnicodeForKeyEvent:
					{
					size_t theLength = sGetParamLength(iEvent, kEventParamTextInputSendText, typeUnicodeText);
					string16 theString16((theLength + 1) / 2, ' ');
					::sGetParam(iEvent, kEventParamTextInputSendText, typeUnicodeText,
						theLength, const_cast<UTF16*>(theString16.data()));

					if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSWindow_Carbon"))
						{
						s << "kEventClassTextInput, kEventTextInputUnicodeForKeyEvent: ";
						string8 theString8 = ZUnicode::sAsUTF8(theString16);
						s << "'" << theString8 << "', ";
						for (string16::iterator i = theString16.begin(); i != theString16.end(); ++i)
							s.Writef("%04X ", uint16(*i));
						s << "UTF8: ";
						for (string8::iterator i = theString8.begin(); i != theString8.end(); ++i)
							s.Writef("%02X ", uint8(*i));
						}
					break;
					}
				}
			break;
			}

#if 1
		case kEventClassKeyboard:
			{
			switch (theEventKind)
				{
				case kEventRawKeyRepeat:
				case kEventRawKeyDown:
					{
					OSErr result = ::CallNextEventHandler(iCallRef, iEvent);
					if (result != eventNotHandledErr)
						return result;

					ZMessage theMessage;
					theMessage.SetString("what", "zoolib:KeyDown");

					UInt32 theKeyCode;
					if (sGetParam_T(iEvent, kEventParamKeyCode, typeUInt32, theKeyCode))
						theMessage.SetInt32("keyCode", theKeyCode);

					char theCharCode;
					if (sGetParam_T(iEvent, kEventParamKeyMacCharCodes, typeChar, theCharCode))
						{
						theMessage.SetInt32("charCode", theCharCode);
						if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSWindow_Carbon"))
							s.Writef("charCode: %c %02X", uint8(theCharCode), uint8(theCharCode));
						}

					UInt32 theModifiers;
					if (sGetParam_T(iEvent, kEventParamKeyModifiers, typeUInt32, theModifiers))
						{
						theMessage.SetBool("shift", theModifiers & shiftKey);
						theMessage.SetBool("command", theModifiers & cmdKey);
						theMessage.SetBool("option", theModifiers & optionKey);
						theMessage.SetBool("control", theModifiers & controlKey);
						theMessage.SetBool("capsLock", theModifiers & alphaLock);
						}

					if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSWindow_Carbon"))
						{
						s << ZString::sFormat("kEventClassKeyboard, WindowRef: %08X, theEventKind: %d", fWindowRef, theEventKind)
							<< theMessage;
						}
					this->DispatchMessageToOwner(theMessage);
					return noErr;
					}
				}
			}
#endif

		case kEventClassMouse:
			{
//			if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSWindow_Carbon"))
//				s << ZString::sFormat("WindowRef: %08X, ", fWindowRef) << sMouseEventToMessage(iEvent);

			switch (theEventKind)
				{
				case kEventMouseDown:
				case kEventMouseUp:
				case kEventMouseMoved:
				case kEventMouseDragged:
					{
					#if ZCONFIG(OS, MacOSX)
						WindowRef theWindowRef = sGetParam_T<WindowRef>(iEvent, kEventParamWindowRef, typeWindowRef);
						WindowPartCode theWPC = sGetParam_T<WindowPartCode>(iEvent, kEventParamWindowPartCode, typeWindowPartCode);
					#else
						Point globalLocation = sGetParam_T<Point>(iEvent, kEventParamMouseLocation, typeQDPoint);
						WindowRef theWindowRef;
						WindowPartCode theWPC = ::MacFindWindow(globalLocation, &theWindowRef);
					#endif
					
					if ((theWindowRef == fWindowRef && theWPC == inContent)
						|| ZOSApp_Carbon::sGet()->IsCaptured(fWindowRef))
						{
						ZMessage theMessage = sMouseEventToMessage(fWindowRef, iEvent);
						if (theEventKind == kEventMouseDown)
							theMessage.SetString("what", "zoolib:MouseDown");
						else if (theEventKind == kEventMouseUp)
							theMessage.SetString("what", "zoolib:MouseUp");
						else if (theEventKind == kEventMouseMoved || theEventKind == kEventMouseDragged)
							theMessage.SetString("what", "zoolib:MouseChange");
							
						if (theEventKind == kEventMouseDown)
							{
							UInt32 clickCount = sGetParam_T<UInt32>(iEvent, kEventParamClickCount, typeUInt32);
							theMessage.SetInt32("clicks", clickCount);
							}
						this->DispatchMessageToOwner(theMessage);
						return noErr;
						}
					}
				}
			break;
			}
		}

	return err;
	}

EventHandlerUPP ZOSWindow_Carbon::sEventHandlerUPP_Content = NewEventHandlerUPP(sEventHandler_Content);

pascal OSStatus ZOSWindow_Carbon::sEventHandler_Content(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{ return static_cast<ZOSWindow_Carbon*>(iRefcon)->EventHandler_Content(iCallRef, iEvent); }

OSStatus ZOSWindow_Carbon::EventHandler_Content(EventHandlerCallRef iCallRef, EventRef iEvent)
	{
	ZMutexLocker locker(fMutex);
	OSStatus err = eventNotHandledErr;

	#if ZCONFIG(OS, MacOSX)
		const UInt32 theEventKind = ::GetEventKind(iEvent);
		switch (::GetEventClass(iEvent))
			{
			case kEventClassControl:
				{
				switch (theEventKind)
					{
					case kEventControlDraw:
						{
						// Shouldn't be called unless we're compositing.
						ZAssert(fCompositing);
						if (fOwner)
							{
							ZDC theDC(this->GetCanvas(), ZPoint::sZero);
							theDC.SetClip(fInvalidRgn);
							theDC.SetInk(fOwner->OwnerGetBackInk(this->GetActive()));
							theDC.Fill(fInvalidRgn);
							fInvalidRgn.Clear();
							theDC.SetInk(ZDCInk());
							fOwner->OwnerUpdate(theDC);
							}
						return noErr;
						}
					}
				break;
				}
			}
	#endif

	return err;	
	}

EventLoopTimerUPP ZOSWindow_Carbon::sEventLoopTimerUPP_Idle = NewEventLoopTimerUPP(sEventLoopTimer_Idle);

pascal void ZOSWindow_Carbon::sEventLoopTimer_Idle(EventLoopTimerRef iTimer, void* iRefcon)
	{ static_cast<ZOSWindow_Carbon*>(iRefcon)->EventLoopTimer_Idle(iTimer); }

void ZOSWindow_Carbon::EventLoopTimer_Idle(EventLoopTimerRef iTimer)
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:Idle");
	
	// MDC STUB This trips the ASSERTLOCKED in ZUITextPane_TextEngine::DoIdle, but I'm not
	// sure this is really the right place to lock.

	// AG 2006-07-17. ZOSWindow_Carbon/ZOSApp_OSX is the first place where we're reverting
	// a a single thread for all UI work. I need to revamp the messaging stuff to accomodate it.
	// This is a reasonable fix for now.
	
	ZLocker locker( GetLock() );
	this->DispatchMessageToOwner(theMessage);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Carbon::OSWindowRoster

class ZOSApp_Carbon::OSWindowRoster : public ZOSWindowRoster
	{
	friend class ZOSApp_Carbon;
public:
	virtual ~OSWindowRoster();

	virtual void Reset();
	virtual bool GetNextOSWindow(bigtime_t iTimeout, ZOSWindow*& oOSWindow);
	virtual void DropCurrentOSWindow();
	virtual void UnlockCurrentOSWindow();

protected:
	ZOSApp_Carbon* fOSApp;
	vector<ZOSWindow_Carbon*> fVisited;
	vector<ZOSWindow_Carbon*> fDropped;
	ZOSWindow_Carbon* fCurrent;
	OSWindowRoster* fPrev;
	OSWindowRoster* fNext;
	};

ZOSApp_Carbon::OSWindowRoster::~OSWindowRoster()
	{
	if (fOSApp)
		fOSApp->OSWindowRoster_Disposed(this);
	}

void ZOSApp_Carbon::OSWindowRoster::Reset()
	{
	if (fOSApp)
		fOSApp->OSWindowRoster_Reset(this);
	}

bool ZOSApp_Carbon::OSWindowRoster::GetNextOSWindow(bigtime_t iTimeout, ZOSWindow*& oOSWindow)
	{
	if (fOSApp)
		return fOSApp->OSWindowRoster_GetNextOSWindow(this, iTimeout, oOSWindow);
	return true;
	}

void ZOSApp_Carbon::OSWindowRoster::DropCurrentOSWindow()
	{
	if (fOSApp)
		return fOSApp->OSWindowRoster_DropCurrentOSWindow(this);
	}

void ZOSApp_Carbon::OSWindowRoster::UnlockCurrentOSWindow()
	{
	if (fOSApp)
		return fOSApp->OSWindowRoster_UnlockCurrentOSWindow(this);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Carbon

static bool sInitedToolbox = false;
ZOSApp_Carbon* ZOSApp_Carbon::sOSApp_Carbon;

ZOSApp_Carbon* ZOSApp_Carbon::sGet()
	{ return sOSApp_Carbon; }

ZOSApp_Carbon::ZOSApp_Carbon()
:	fMutex("ZOSApp_Carbon::fMutex", true),
	fIsRunning(false),
	fPostedRunStarted(false),
	fEventHandlerRef_Dispatcher(nil),
	fEventHandlerRef_Application(nil),
	fEventLoopTimerRef_Idle(nil),
	fWindowRef_Capture(nil),
	fOSWindowRoster_Head(nil)
	{
	ZAssertStop(kDebug_Carbon, sOSApp_Carbon == nil);
	sOSApp_Carbon = this;

	if (!sInitedToolbox)
		{
		// Initialize the toolbox, as we have not already done so.
		sInitedToolbox = true;

		// Install the AppleEvent handler for display manager events
		::AEInstallEventHandler(kCoreEventClass, kAESystemConfigNotice, sAppleEventHandlerUPP, kAESystemConfigNotice, false);

		// And the rest of the core suite
		::AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, sAppleEventHandlerUPP, kAEOpenApplication, false);
		::AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, sAppleEventHandlerUPP, kAEReopenApplication, false);
		::AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, sAppleEventHandlerUPP, kAEOpenDocuments, false);
		::AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, sAppleEventHandlerUPP, kAEPrintDocuments, false);
		::AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, sAppleEventHandlerUPP, kAEQuitApplication, false);
		}

	// ----------

	static EventTypeSpec sEvents_Dispatcher[] =
		{
		{ kEventClassWindow, kEventWindowShown },
		{ kEventClassWindow, kEventWindowHidden },
		{ kEventClassWindow, kEventWindowActivated },
		{ kEventClassWindow, kEventWindowDeactivated },
		{ kEventClassWindow, kEventWindowCursorChange },

		{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },

		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },

		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, 11 }, // MightyMouse wheels
		{ kEventClassMouse, kEventMouseWheelMoved },
		#if ZCONFIG(OS, MacOSX)
			{ kEventClassMouse, kEventMouseEntered },
			{ kEventClassMouse, kEventMouseExited },
		#endif
		};

	::InstallEventHandler(::GetEventDispatcherTarget(), sEventHandlerUPP_Dispatcher,
		countof(sEvents_Dispatcher), sEvents_Dispatcher,
		this, &fEventHandlerRef_Dispatcher);


	// ----------

	static const EventTypeSpec sEvents_Application[] =
		{
		{ kEventClassWindow, kEventWindowClosed },

		{ kEventClassCommand, kEventCommandProcess },
//		{ kEventClassCommand, kEventCommandUpdateStatus },

		{ kEventClassMenu, kEventMenuBeginTracking },

		{ kEventClassID_ZooLib, kEventKindID_Message }
		};

	::InstallEventHandler(::GetApplicationEventTarget(), sEventHandlerUPP_Application,
		countof(sEvents_Application), sEvents_Application,
		this, &fEventHandlerRef_Application);

	// ----------

	::InstallEventLoopTimer(::GetCurrentEventLoop(), 0.1, 0.1,
		sEventLoopTimerUPP_Idle, this, &fEventLoopTimerRef_Idle);

	#if ZCONFIG(OS, MacOSX)
		sInitTraceEvents();
	#endif

	ZDCScratch::sSet(new ZDCCanvas_QD_OffScreen(ZPoint(2,1), ZDCPixmapNS::eFormatEfficient_Color_32));
	}

ZOSApp_Carbon::~ZOSApp_Carbon()
	{
	ZMessage theDieMessage;
	theDieMessage.SetString("what", "zoolib:DieDieDie");

	for (;;)
		{
		bool hitAny = false;
		for (WindowRef iterWindowRef = ::GetWindowList(); iterWindowRef; iterWindowRef = ::MacGetNextWindow(iterWindowRef))
			{
			if (ZOSWindow_Carbon* currentOSWindow = sFromWindowRef(iterWindowRef))
				{
				currentOSWindow->DispatchMessage(theDieMessage);
				hitAny = true;
				}
			}
		if (!hitAny)
			break;
		}

	while (fOSWindowRoster_Head)
		{
		OSWindowRoster* next = fOSWindowRoster_Head->fNext;
		fOSWindowRoster_Head->fOSApp = nil;
		fOSWindowRoster_Head->fNext = nil;
		fOSWindowRoster_Head->fPrev = nil;
		fOSWindowRoster_Head = next;
		}

	if (fEventLoopTimerRef_Idle)
		::RemoveEventLoopTimer(fEventLoopTimerRef_Idle);
	fEventLoopTimerRef_Idle = nil;

	if (fEventHandlerRef_Application)
		::RemoveEventHandler(fEventHandlerRef_Application);
	fEventHandlerRef_Application = nil;

	if (fEventHandlerRef_Dispatcher)
		::RemoveEventHandler(fEventHandlerRef_Dispatcher);
	fEventHandlerRef_Dispatcher = nil;

	ZDCScratch::sSet(ZRef<ZDCCanvas>());

	ZAssertStop(kDebug_Carbon, sOSApp_Carbon == this);
	sOSApp_Carbon = nil;
	}

ZMutexBase& ZOSApp_Carbon::GetLock()
	{
	return fMutex;
	}

void ZOSApp_Carbon::QueueMessage(const ZMessage& iMessage)
	{
	EventRef theEventRef;
	if (noErr == ::MacCreateEvent(nil, kEventClassID_ZooLib, kEventKindID_Message,
		::GetCurrentEventTime(), kEventAttributeNone, &theEventRef))
		{
		ZMessage* theMessage = new ZMessage(iMessage);
		::SetEventParameter(theEventRef, 'Mess', typePtr, sizeof(theMessage), &theMessage);
		::PostEventToQueue(::GetMainEventQueue(), theEventRef, kEventPriorityStandard);
		::ReleaseEvent(theEventRef);
		}
	else
		{
		throw bad_alloc();
		}
	}

bool ZOSApp_Carbon::DispatchMessage(const ZMessage& iMessage)
	{
	return ZOSApp::DispatchMessage(iMessage);
	}

#if ZCONFIG(API_Thread, Mac)

static pascal OSStatus sEventHandler_Quit(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{
	OSStatus err = CallNextEventHandler(iCallRef, iEvent);
	if (err == noErr)
		*static_cast<bool*>(iRefcon) = true;
	return err;
	}

static EventHandlerUPP sEventHandlerUPP_Quit = NewEventHandlerUPP(sEventHandler_Quit);

static pascal OSStatus sEventHandler_RAEL(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{
	static EventTypeSpec sEvents_Quit[] =
		{
		{ kEventClassApplication, kEventAppQuit }
		};
	
	bool quitNow = false;
	EventHandlerRef installedHandler;
	if (noErr == ::InstallEventHandler(::GetApplicationEventTarget(), sEventHandlerUPP_Quit,
		countof(sEvents_Quit), sEvents_Quit, &quitNow, &installedHandler))
		{
		EventTargetRef theEventDispatcherTarget = ::GetEventDispatcherTarget();
		while (!quitNow)
			{
			EventRef theEventRef;
			OSStatus theErr = ZThreadTM_ReceiveNextEvent(false, 0, nil, &theEventRef);
			if (theErr == noErr)
				{
				::SendEventToEventTarget(theEventRef, theEventDispatcherTarget);
				::ReleaseEvent(theEventRef);
				}
			}
		::RemoveEventHandler(installedHandler);
		}
	return noErr;
	}

static EventHandlerUPP sEventHandlerUPP_RAEL = NewEventHandlerUPP(sEventHandler_RAEL);

static void sRunApplicationEventLoop()
	{
	static EventTypeSpec sEvents_RAEL[] =
		{
		{ kEventClassID_ZooLib, kEventKindID_RAEL }
		};

	EventHandlerRef installedHandler;
	if (noErr == ::InstallEventHandler(::GetApplicationEventTarget(), sEventHandlerUPP_RAEL,
		countof(sEvents_RAEL), sEvents_RAEL, nil, &installedHandler))
		{
		EventRef theEventRef;
		if (noErr == ::MacCreateEvent(nil, kEventClassID_ZooLib, kEventKindID_RAEL,
			::GetCurrentEventTime(), kEventAttributeNone, &theEventRef))
			{
			if (noErr == ::PostEventToQueue(::GetMainEventQueue(), theEventRef, kEventPriorityHigh))
				::RunApplicationEventLoop();
			::ReleaseEvent(theEventRef);
			}
		::RemoveEventHandler(installedHandler);
		}
	}
#endif // ZCONFIG(API_Thread, Mac)

void ZOSApp_Carbon::Run()
	{
	ZAssertStop(kDebug_Carbon, !fIsRunning);
	fIsRunning = true;

	fMutex.Release();

	#if ZCONFIG(API_Thread, Mac)
		::sRunApplicationEventLoop();
	#else
		::RunApplicationEventLoop();
	#endif

	fMutex.Acquire();

	ZAssertStop(kDebug_Carbon, fIsRunning);
	fIsRunning = false;
	}

bool ZOSApp_Carbon::IsRunning()
	{
	return fIsRunning;
	}

void ZOSApp_Carbon::ExitRun()
	{
	::QuitApplicationEventLoop();
	}

string ZOSApp_Carbon::GetAppName()
	{
	return "ZOSApp_Carbon::GetAppName()";
	}

bool ZOSApp_Carbon::HasUserAccessibleWindows()
	{
	ZDebugLog(0);
	return true;
	}

bool ZOSApp_Carbon::HasGlobalMenuBar()
	{ return true; }

void ZOSApp_Carbon::SetMenuBar(const ZMenuBar& iMenuBar)
	{
	fMenuBar = iMenuBar;
	if (nil == this->pGetActiveWindow())
		this->pSetMenuBar(fMenuBar);
	}

void ZOSApp_Carbon::BroadcastMessageToAllWindows(const ZMessage& iMessage)
	{
	ZDebugLog(0);
	}

ZOSWindow* ZOSApp_Carbon::CreateOSWindow(const ZOSWindow::CreationAttributes& iAttributes)
	{
	WindowRef theWindowRef = ZOSWindow_Carbon::sCreateWindowRef(iAttributes);
	return new ZOSWindow_Carbon(theWindowRef);
	}

ZOSWindowRoster* ZOSApp_Carbon::CreateOSWindowRoster()
	{
	OSWindowRoster* theRoster = new OSWindowRoster;
	theRoster->fOSApp = this;

	if (fOSWindowRoster_Head)
		fOSWindowRoster_Head->fPrev = theRoster;

	theRoster->fNext = fOSWindowRoster_Head;
	theRoster->fPrev = nil;
	fOSWindowRoster_Head = theRoster;
	return theRoster;
	}

ZOSWindow_Carbon* ZOSApp_Carbon::GetActiveOSWindow()
	{ return this->pGetActiveWindow(); }

void ZOSApp_Carbon::AcquireCapture(WindowRef iWindowRef)
	{
	if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::AcquireCapture"))
		s << ZString::sFormat("iWindowRef: %08X, fWindowRef_Capture: %08X", iWindowRef, fWindowRef_Capture);
	if (fWindowRef_Capture != iWindowRef)
		{
		WindowRef oldWindowRef = fWindowRef_Capture;
		fWindowRef_Capture = iWindowRef;
		if (ZOSWindow_Carbon* theOSWindow = sFromWindowRef(oldWindowRef))
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:CaptureCancelled");
			theOSWindow->DispatchMessageToOwner(theMessage);
			}		
		}
	}

void ZOSApp_Carbon::ReleaseCapture(WindowRef iWindowRef)
	{
	if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::ReleaseCapture"))
		s << ZString::sFormat("iWindowRef: %08X, fWindowRef_Capture: %08X", iWindowRef, fWindowRef_Capture);
	if (fWindowRef_Capture && fWindowRef_Capture == iWindowRef)
		{
		fWindowRef_Capture = nil;
		if (ZOSWindow_Carbon* theOSWindow = sFromWindowRef(iWindowRef))
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:CaptureReleased");
			theOSWindow->DispatchMessageToOwner(theMessage);
			}		
		}
	}

void ZOSApp_Carbon::CancelCapture()
	{
	if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::CancelCapture"))
		s << ZString::sFormat("fWindowRef_Capture: %08X", fWindowRef_Capture);
	this->AcquireCapture(nil);
	}

bool ZOSApp_Carbon::IsCaptured(WindowRef iWindowRef)
	{
	return fWindowRef_Capture == iWindowRef;
	}

void ZOSApp_Carbon::MenuBarChanged(ZOSWindow_Carbon* iWindow)
	{
	if (iWindow == this->pGetActiveWindow())
		this->pSetMenuBar(iWindow->fMenuBar);
	}

ZOSWindow_Carbon* ZOSApp_Carbon::pGetActiveWindow()
	{
	return sFromWindowRef(::GetUserFocusWindow());	
	}

void ZOSApp_Carbon::pSetMenuBar(const ZMenuBar& iMenuBar)
	{
	ZMenu_Carbon::sInstallMenuBar(iMenuBar);
	}

EventHandlerUPP ZOSApp_Carbon::sEventHandlerUPP_Dispatcher = NewEventHandlerUPP(sEventHandler_Dispatcher);

pascal OSStatus ZOSApp_Carbon::sEventHandler_Dispatcher(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{ return static_cast<ZOSApp_Carbon*>(iRefcon)->EventHandler_Dispatcher(iCallRef, iEvent); }

void ZOSApp_Carbon::Disposed(ZOSWindow_Carbon* iOSWindow)
	{
	for (OSWindowRoster* iter = fOSWindowRoster_Head; iter; iter = iter->fNext)
		{
		if (iter->fCurrent == iOSWindow)
			iter->fCurrent = nil;
		ZUtil_STL::sEraseIfContains(iter->fDropped, iOSWindow);
		ZUtil_STL::sEraseIfContains(iter->fVisited, iOSWindow);
		}
	}

OSStatus ZOSApp_Carbon::EventHandler_Dispatcher(EventHandlerCallRef iCallRef, EventRef iEvent)
	{
	ZMutexLocker locker(fMutex);
	OSStatus err = eventNotHandledErr;

	const UInt32 theEventKind = ::GetEventKind(iEvent);
	switch (::GetEventClass(iEvent))
		{
		case kEventClassID_ZooLib:
			{
			switch (theEventKind)
				{
				case kEventKindID_Message:
					{
					ZMessage* theMessage;
					if (sGetParam_T(iEvent, 'Mess', typePtr, theMessage))
						{
						::CallNextEventHandler(iCallRef, iEvent);
						delete theMessage;
						return noErr;
						}
					}
				}
			break;
			}

		case kEventClassWindow:
			{
			switch (theEventKind)
				{
				case kEventWindowCursorChange:
					{
					::InitCursor();
					return noErr;
					}
				}
			break;
			}

		case kEventClassMenu:
			{
			if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::EventHandler_Dispatcher"))
				s << ZString::sFormat("kEventClassMenu, theEventKind: %d", theEventKind);
			break;
			}

		case kEventClassTextInput:
			{
			switch (theEventKind)
				{
				case kEventTextInputUnicodeForKeyEvent:
					{
					if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::EventHandler_Dispatcher"))
						s << "kEventClassTextInput, kEventTextInputUnicodeForKeyEvent";

					if (fWindowRef_Capture)
						return ::SendEventToEventTarget(iEvent, ::GetWindowEventTarget(fWindowRef_Capture));
					break;
					}
				}
			break;
			}

		case kEventClassKeyboard:
			{
			if (fWindowRef_Capture)
				return ::SendEventToEventTarget(iEvent, ::GetWindowEventTarget(fWindowRef_Capture));
			break;
			}

		case kEventClassMouse:
			{
			if (fWindowRef_Capture)
				{
				if (theEventKind == kEventMouseDown)
					{
					#if ZCONFIG(OS, MacOSX)
						WindowRef theWindowRef = sGetParam_T<WindowRef>(iEvent, kEventParamWindowRef, typeWindowRef);
						WindowPartCode theWPC = sGetParam_T<WindowPartCode>(iEvent, kEventParamWindowPartCode, typeWindowPartCode);
					#else
						Point globalLocation = sGetParam_T<Point>(iEvent, kEventParamMouseLocation, typeQDPoint);
						WindowRef theWindowRef;
						WindowPartCode theWPC = ::MacFindWindow(globalLocation, &theWindowRef);
					#endif

					if (fWindowRef_Capture != theWindowRef || inContent != theWPC)
						{
						// That's not occurring in the capture window, or is not in
						// the capture window's content area, so we cancel the capture.
						this->CancelCapture();
						}
					}
				}

			if (fWindowRef_Capture)
				{
				if (fWindowRef_Capture != sGetParam_T<WindowRef>(iEvent, kEventParamWindowRef, typeWindowRef))
					{
					// The window ref (if any) is not the capture window and
					// so would not naturally receive the event, so we send
					// it explicitly.
					return ::SendEventToEventTarget(iEvent, ::GetWindowEventTarget(fWindowRef_Capture));
					}
				}
			break;
			}
		}
	return err;
	}

EventHandlerUPP ZOSApp_Carbon::sEventHandlerUPP_Application = NewEventHandlerUPP(sEventHandler_Application);

pascal OSStatus ZOSApp_Carbon::sEventHandler_Application(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon)
	{ return static_cast<ZOSApp_Carbon*>(iRefcon)->EventHandler_Application(iCallRef, iEvent); }

static ZRef<ZMenuItem> sGetMenuItem(EventRef iEvent)
	{
	HICommand theHICommand;
	if (sGetParam_T<HICommand>(iEvent, kEventParamDirectObject, typeHICommand, theHICommand))
		return ZMenu_Carbon::sGetMenuItem(theHICommand.menu.menuRef, theHICommand.menu.menuItemIndex);
	return ZRef<ZMenuItem>();
	}

OSStatus ZOSApp_Carbon::EventHandler_Application(EventHandlerCallRef iCallRef, EventRef iEvent)
	{
	ZMutexLocker locker(fMutex);
	OSStatus err = eventNotHandledErr;

	const UInt32 theEventKind = ::GetEventKind(iEvent);
	switch (::GetEventClass(iEvent))
		{
		case kEventClassWindow:
			{
			switch (theEventKind)
				{
				case kEventWindowClosed:
					{
					WindowRef theWindowRef;
					if (sGetParam_T(iEvent, kEventParamDirectObject, typeWindowRef, theWindowRef))
						{
						if (fWindowRef_Capture == theWindowRef)
							fWindowRef_Capture = nil;
						}
					break;
					}
				}
			break;
			}

		case kEventClassID_ZooLib:
			{
			switch (theEventKind)
				{
				case kEventKindID_Message:
					{
					ZMessage* theMessage;
					if (sGetParam(iEvent, 'Mess', typePtr, sizeof(theMessage), &theMessage))
						{
						if (theMessage)
							{
							try
								{
								this->DispatchMessage(*theMessage);
								}
							catch (...)
								{}
							}
						}
					return noErr;
					}
				}
			break;
			}

		case kEventClassMenu:
			{
			if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::EventHandler_Application"))
				s << ZString::sFormat("kEventClassMenu, theEventKind: %d", theEventKind);

			switch (theEventKind)
				{
				case kEventMenuBeginTracking:
					{
					if (ZOSWindow_Carbon* theOSWindow = this->pGetActiveWindow())
						{
						theOSWindow->fMenuBar.Reset();
						theOSWindow->GetOwner()->OwnerSetupMenus(theOSWindow->fMenuBar);
						ZMenu_Carbon::sSetupMenuBar(theOSWindow->fMenuBar);
						}
					else
						{
						fMenuBar.Reset();
						if (fOwner)
							fOwner->OwnerSetupMenus(fMenuBar);
						ZMenu_Carbon::sSetupMenuBar(fMenuBar);
						}
					}
				}
			break;
			}

		case kEventClassCommand:
			{
			if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZOSApp_Carbon::EventHandler_Application"))
				s << ZString::sFormat("kEventClassCommand, theEventKind: %d", theEventKind);

			switch (theEventKind)
				{
				case kEventCommandProcess:
					{
					if (ZRef<ZMenuItem> theMenuItem = sGetMenuItem(iEvent))
						{		
						ZMessage theMessage = theMenuItem->GetMessage();
						theMessage.SetString("what", "zoolib:Menu");
						theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
						if (ZOSWindow_Carbon* theOSWindow = this->pGetActiveWindow())
							theOSWindow->DispatchMessageToOwner(theMessage);
						else
							this->DispatchMessageToOwner(theMessage);

						return noErr;
						}
					}		
				}
			break;
			}
		}
	return err;
	}


AEEventHandlerUPP ZOSApp_Carbon::sAppleEventHandlerUPP = NewAEEventHandlerUPP(ZOSApp_Carbon::sAppleEventHandler);

DEFINE_API(OSErr) ZOSApp_Carbon::sAppleEventHandler(const AppleEvent* iMessage, AppleEvent* oReply, long iRefcon)
	{
	ZAssertStop(kDebug_Carbon, ZOSApp_Carbon::sGet());
	return ZOSApp_Carbon::sGet()->AppleEventHandler(iMessage, oReply, iRefcon);
	}

OSErr ZOSApp_Carbon::AppleEventHandler(const AppleEvent* iMessage, AppleEvent* oReply, unsigned long iRefcon)
	{
	switch (iRefcon)
		{
		case kAESystemConfigNotice:
			{
			ZMutexLocker structureLocker(fMutex);
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:ScreenLayoutChanged");
			this->BroadcastMessageToAllWindows(theMessage);
			break;
			}
		case kAEOpenApplication:
			{
			ZAssertStop(kDebug_Carbon, !fPostedRunStarted);
			fPostedRunStarted = true;
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RunStarted");
			this->DispatchMessageToOwner(theMessage);
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

			AEDescList listOfFiles;

			if (noErr == ::AEGetParamDesc(iMessage, keyDirectObject, typeAEList, &listOfFiles))
				{
				long numFiles;
				if (noErr == ::AECountItems(&listOfFiles, &numFiles))
					{
					for (long i = 1; i <= numFiles; ++i)
						{
						AEKeyword theActualKey;
						DescType theActualType;
						FSSpec theFSSpec;
						Size theActualSize;

						if (noErr == ::AEGetNthPtr(&listOfFiles, i, typeFSS,
									&theActualKey, &theActualType,
									(Ptr) &theFSSpec, sizeof(theFSSpec), &theActualSize))
							{
							theMessage.AppendRefCounted("files", new ZFileLoc_Mac_FSSpec(theFSSpec));
							}
						}
					::AEDisposeDesc(&listOfFiles);
					}
				}
			this->DispatchMessageToOwner(theMessage);

			if (!fPostedRunStarted)
				{
				fPostedRunStarted = true;
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:RunStarted");
				this->DispatchMessageToOwner(theMessage);
				}
			break;
			}
		case kAEPrintDocuments:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:PrintDocument");
			this->DispatchMessageToOwner(theMessage);

			if (!fPostedRunStarted)
				{
				fPostedRunStarted = true;
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:RunStarted");
				this->DispatchMessageToOwner(theMessage);
				}
			break;
			}
		case kAEQuitApplication:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RequestQuit");
			this->DispatchMessageToOwner(theMessage);
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

EventLoopTimerUPP ZOSApp_Carbon::sEventLoopTimerUPP_Idle = NewEventLoopTimerUPP(sEventLoopTimer_Idle);

pascal void ZOSApp_Carbon::sEventLoopTimer_Idle(EventLoopTimerRef iTimer, void* iRefcon)
	{ static_cast<ZOSApp_Carbon*>(iRefcon)->EventLoopTimer_Idle(iTimer); }

void ZOSApp_Carbon::EventLoopTimer_Idle(EventLoopTimerRef iTimer)
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:Idle");
	this->DispatchMessageToOwner(theMessage);
	}

void ZOSApp_Carbon::OSWindowRoster_Disposed(OSWindowRoster* iRoster)
	{
	if (iRoster->fNext)
		iRoster->fNext->fPrev = iRoster->fPrev;
	if (iRoster->fPrev)
		iRoster->fPrev->fNext = iRoster->fNext;
	if (fOSWindowRoster_Head == iRoster)
		fOSWindowRoster_Head = iRoster->fNext;
	iRoster->fPrev = nil;
	iRoster->fNext = nil;
	}

void ZOSApp_Carbon::OSWindowRoster_Reset(OSWindowRoster* iRoster)
	{
	iRoster->fVisited.clear();
	}

bool ZOSApp_Carbon::OSWindowRoster_GetNextOSWindow(OSWindowRoster* iRoster, bigtime_t iTimeout, ZOSWindow*& outOSWindow)
	{
	outOSWindow = nil;
	bigtime_t endTime = ZTicks::sNow() + iTimeout;
	for (WindowRef iterWindowRef = ::GetWindowList(); iterWindowRef; iterWindowRef = ::MacGetNextWindow(iterWindowRef))
		{
		if (ZOSWindow_Carbon* currentOSWindow = sFromWindowRef(iterWindowRef))
			{
			if (!ZUtil_STL::sContains(iRoster->fVisited, currentOSWindow))
				{
				if (!ZUtil_STL::sContains(iRoster->fDropped, currentOSWindow))
					{
					ZThread::Error err = currentOSWindow->GetLock().Acquire(endTime - ZTicks::sNow());
					if (err == ZThread::errorTimeout)
						return false;
					if (err == ZThread::errorNone)
						{
						iRoster->fCurrent = currentOSWindow;
						iRoster->fVisited.push_back(currentOSWindow);
						outOSWindow = currentOSWindow;
						return true;
						}
					}
				}
			}
		}
	return true;
	}

void ZOSApp_Carbon::OSWindowRoster_DropCurrentOSWindow(OSWindowRoster* iRoster)
	{
	if (!iRoster->fCurrent)
		return;
	iRoster->fDropped.push_back(iRoster->fCurrent);
	}

void ZOSApp_Carbon::OSWindowRoster_UnlockCurrentOSWindow(OSWindowRoster* iRoster)
	{
	if (!iRoster->fCurrent)
		return;
	iRoster->fCurrent->GetLock().Release();
	iRoster->fCurrent = nil;
	}

#endif // ZCONFIG(API_OSWindow, Carbon)

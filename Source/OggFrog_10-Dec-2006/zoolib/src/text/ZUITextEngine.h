/*  @(#) $Id: ZUITextEngine.h,v 1.5 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUITextEngine__
#define __ZUITextEngine__ 1
#include "zconfig.h"

#include <string>
#include "ZUI.h"
#include "ZDC.h"
#include "ZEvent.h"
#include "ZDragClip.h"

// ZUITextEngine is an abstract interface to a text editing engine like TextEdit or Paige,
// one that can be dynamically placed in any ZDC. As this may be hard to support for some
// text editing solutions (like BeOS TextViews or Windows edit controls) we also define
// an abstract ZUITextPane -- a pane that can do text editing.
// =================================================================================================
class ZUITextEngine : public ZUITextContainer,
								public ZDragSource,
								public ZUIScrollable,
								public ZUISelection
	{
public:
	class Host;
	class CheckScroll;

	ZUITextEngine();
	virtual ~ZUITextEngine();

	virtual void Attached(Host* inHost);
	virtual void Detached(Host* inHost);

	Host* GetHost();

	ZDC GetHostDC();

	virtual void Draw(const ZDC& inDC) = 0;
	virtual void DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength);
	virtual bool Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool KeyDown(const ZEvent_Key& inEvent) = 0;
	virtual void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);
	virtual bool GetCursor(ZPoint inPoint, ZCursor& outCursor);
	virtual void Activate(bool entering) = 0;
	virtual void Idle(const ZDC& inDC, bool inEnabledAndActive) = 0;

	virtual void SetDragOffset(size_t inDragOffset, bool inDragActive);
	virtual void SetDragBorderVisible(bool inVisible);

	virtual void SetupMenus(ZMenuSetup* inMenuSetup);
	virtual bool MenuMessage(const ZMessage& inMessage);

	virtual void SetFont(const ZDCFont& inFont) = 0;

	virtual void SetTextColor(ZRGBColor textColor) = 0;
	virtual ZRGBColor GetTextColor() = 0;
	virtual void SetBackColor(ZRGBColor backColor) = 0;
	virtual ZRGBColor GetBackColor() = 0;

	virtual size_t PointToOffset(ZPoint inPoint) = 0;

	virtual ZDCRgn GetTextRgn(size_t inOffset, size_t inLength);

	virtual void HandleDrag(ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag);
	virtual void HandleDrop(ZPoint inPoint, ZDrop& inDrop);
	virtual bool AcceptsDragDrop(const ZDragDropBase& inDragDrop);

	virtual ZTuple GetTuple(size_t inOffset, size_t inLength);

	virtual long GetUserChangeCount() = 0;

	virtual bool IsRangeEditable(size_t inOffset, size_t inLength);

	ZPoint FromHost(ZPoint inHostPoint);
	ZPoint ToHost(ZPoint inHostPoint);
protected:
	Host* fHost;
	};

class ZUITextEngine::CheckScroll
	{
public:
	CheckScroll(ZUITextEngine* inTextEngine);
	~CheckScroll();
protected:
	ZUITextEngine* fTextEngine;
	ZPoint_T<double> fOldThumbProportions;
	ZPoint_T<double> fOldScrollValues;
	};

class ZUITextEngine::Host
	{
public:
	virtual ZSubPane* GetHostPane(ZUITextEngine* inTextEngine) = 0;
	virtual ZPoint GetEngineLocation(ZUITextEngine* inTextEngine) = 0;
	virtual void GetEngineSizes(ZUITextEngine* inTextEngine, ZPoint& outEngineSize, ZCoord& outEngineWidth) = 0;
	virtual ZDCRgn GetEngineClip(ZUITextEngine* inTextEngine) = 0;
	virtual bool GetEngineEditable(ZUITextEngine* inTextEngine) = 0;
	virtual bool GetEngineActive(ZUITextEngine* inTextEngine) = 0;
	};

// =================================================================================================
// A text pane that uses a text engine to do its work
class ZUITextPane_TextEngine : public ZUITextPane,
				public ZUITextEngine::Host,
				public ZUIScrollable::Responder,
				public ZUISelection::Responder,
				public ZDragDropHandler
	{
public:
	ZUITextPane_TextEngine(ZSuperPane* superPane, ZPaneLocator* paneLocator, ZUITextEngine* inTextEngine);
	virtual ~ZUITextPane_TextEngine();

// From ZUITextEngine::Host
	virtual ZSubPane* GetHostPane(ZUITextEngine* inTextEngine);
	virtual ZPoint GetEngineLocation(ZUITextEngine* inTextEngine);
	virtual void GetEngineSizes(ZUITextEngine* inTextEngine, ZPoint& outEngineSize, ZCoord& outEngineWidth);
	virtual ZDCRgn GetEngineClip(ZUITextEngine* inTextEngine);
	virtual bool GetEngineEditable(ZUITextEngine* inTextEngine);
	virtual bool GetEngineActive(ZUITextEngine* inTextEngine);

// From ZUIScrollable::Responder
	virtual void ScrollableChanged(ZUIScrollable* inScrollable);

// From ZUISelection::Responder
	virtual void HandleUISelectionEvent(ZUISelection* inSelection, ZUISelection::Event inEvent);

// From ZUITextPane
	virtual long GetUserChangeCount();

// Our protocol
	ZUITextEngine* GetTextEngine();
	void SetTextEngine(ZUITextEngine* inTextEngine);

// From ZUITextContainer by way of ZUITextPane
	virtual size_t GetTextLength();
	virtual void GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength);

	virtual void SetText_Impl(const char* inSource, size_t inLength);

	virtual void InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength);
	virtual void DeleteText(size_t inOffset, size_t inLength);

	virtual void ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength);

	virtual void GetSelection(size_t& outOffset, size_t& outLength);
	virtual void SetSelection(size_t inOffset, size_t inLength);

	virtual void ReplaceSelection_Impl(const char* inSource, size_t inLength);

// From ZUIScrollable by way of ZTUTextPane
	virtual void ScrollBy(ZPoint_T<double> delta, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollTo(ZPoint_T<double> newValues, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollStep(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollPage(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications);

	virtual ZPoint_T<double> GetScrollValues();
	virtual ZPoint_T<double> GetScrollThumbProportions();

// From ZUITextPane
	virtual void SetFont(const ZDCFont& inFont);

// From ZSubPane via ZUITextPane
	virtual void DoIdle();

// From ZDragDropHandler
	virtual void HandleDrag(ZSubPane* inSubPane, ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag);
	virtual void HandleDrop(ZSubPane* inSubPane, ZPoint inPoint, ZDrop& inDrop);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);
	virtual void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);
	virtual bool GetCursor(ZPoint inPoint, ZCursor& outCursor);
	virtual void Activate(bool entering);

	virtual ZDragDropHandler* GetDragDropHandler();

	virtual void DoSetupMenus(ZMenuSetup* inMenuSetup);
	virtual bool DoMenuMessage(const ZMessage& inMessage);

// From ZEventHr (and ZSubPane)
	virtual void BecomeTabTarget(bool inTabbingBackwards);
	virtual bool WantsToBecomeTarget();

	virtual void HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual void HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);

protected:
	class Tracker;
	friend class Tracker;
	ZUITextEngine* fTextEngine;
	};

#endif // __ZUITextEngine__

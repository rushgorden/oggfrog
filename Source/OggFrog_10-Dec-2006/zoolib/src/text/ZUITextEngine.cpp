static const char rcsid[] = "@(#) $Id: ZUITextEngine.cpp,v 1.8 2006/07/12 19:35:40 agreen Exp $";

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

#include "ZUITextEngine.h"
#include "ZFakeWindow.h" // For ZMouseTracker

ZUITextEngine::ZUITextEngine()
:	fHost(nil)
	{}

ZUITextEngine::~ZUITextEngine()
	{}

void ZUITextEngine::Attached(Host* inHost)
	{
	ZAssert(fHost == nil);
	fHost = inHost;
	}

void ZUITextEngine::Detached(Host* inHost)
	{
	ZAssert(fHost == inHost);
	fHost = nil;
	}

ZDC ZUITextEngine::GetHostDC()
	{
	ZDC theDC = fHost->GetHostPane(this)->GetDC();
	theDC.SetClip(fHost->GetEngineClip(this));
	theDC.OffsetOrigin(fHost->GetEngineLocation(this));
	return theDC;
	}

void ZUITextEngine::DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength)
	{
	this->Draw(inDC);
	}

bool ZUITextEngine::Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	size_t selectStart, selectLength;
	this->GetSelection(selectStart, selectLength);
	if (selectLength != 0)
		{
		size_t cursorOffset = this->PointToOffset(this->FromHost(inHitPoint));
		if (cursorOffset >= selectStart && cursorOffset < selectStart + selectLength)
			{
			if (fHost->GetHostPane(this)->GetWindow()->WaitForMouse())
				{
				ZTuple theTuple = this->GetTuple(selectStart, selectLength);
				if (ZDragInitiator* theDragInitiator = fHost->GetHostPane(this)->GetWindow()->CreateDragInitiator())
					{
					ZDC theDC = this->GetHostDC();
					ZDCRgn hiliteRgn = theDC.GetClip() & this->GetTextRgn(selectStart, selectLength);
					theDC.SetClip(hiliteRgn);

					ZDCPixmap thePixmap;
					ZPoint hiliteRgnSize = hiliteRgn.Bounds().Size();
					if ((hiliteRgnSize.h * hiliteRgnSize.v) <= 90000)
						{
						ZDC_Off theOffDC(theDC, true);
						this->DrawRange(theOffDC, selectStart, selectLength);
						theOffDC.Sync();
						thePixmap = theOffDC.GetPixmap(theOffDC.GetClip().Bounds());
						}
					ZPoint theOffset = hiliteRgn.Bounds().TopLeft();
					hiliteRgn -= theOffset;
					theDragInitiator->DoDrag(theTuple, fHost->GetHostPane(this)->ToGlobal(inHitPoint), this->FromHost(inHitPoint) - theOffset, theDragInitiator->OutlineRgn(hiliteRgn), thePixmap, hiliteRgn, this);
					return true;
					}
				}
			}
		}
	return false;
	}

ZUITextEngine::Host* ZUITextEngine::GetHost()
	{ return fHost; }

void ZUITextEngine::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	if (inMotion == ZMouse::eMotionEnter || inMotion == ZMouse::eMotionLeave || inMotion == ZMouse::eMotionMove)
		fHost->GetHostPane(this)->InvalidateCursor();
	}

bool ZUITextEngine::GetCursor(ZPoint inPoint, ZCursor& outCursor)
	{
	size_t selectStart, selectLength;
	this->GetSelection(selectStart, selectLength);
	if (selectLength != 0)
		{
		size_t cursorOffset = this->PointToOffset(this->FromHost(inPoint));
		if (cursorOffset >= selectStart && cursorOffset < selectStart + selectLength)
			{
			outCursor.Set(ZCursor::eCursorTypeArrow);
			return true;
			}
		}
	outCursor.Set(ZCursor::eCursorTypeIBeam);
	return true;
	}

bool ZUITextEngine::AcceptsDragDrop(const ZDragDropBase& inDragDrop)
	{
	bool gotIt = inDragDrop.GetDragSource() == this;
	for (size_t currentTupleIndex = 0; !gotIt && currentTupleIndex < inDragDrop.CountTuples(); ++currentTupleIndex)
		{
		ZTuple currentTuple = inDragDrop.GetTuple(currentTupleIndex);
		if (currentTuple.Has("text/plain"))
			gotIt = true;
		}
	return gotIt;
	}

void ZUITextEngine::HandleDrag(ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag)
	{
	if (!this->AcceptsDragDrop(inDrag))
		return;
	if (inMotion == ZMouse::eMotionEnter || inMotion == ZMouse::eMotionMove)
		{
		bool dragEnabled = true;
		size_t dragOffset = this->PointToOffset(this->FromHost(inPoint));
		if (inDrag.GetDragSource() == this)
			{
			size_t selectStart, selectLength;
			this->GetSelection(selectStart, selectLength);
			if (dragOffset >= selectStart && dragOffset <= selectStart+selectLength)
				dragEnabled = false;
			}
		if (dragEnabled)
			{
			this->SetDragBorderVisible(true);
			this->SetDragOffset(dragOffset, true);
			}
		else
			{
			this->SetDragOffset(dragOffset, false);
			this->SetDragBorderVisible(false);
			}
		}
	if (inMotion == ZMouse::eMotionLeave)
		{
		this->SetDragOffset(0, false);
		this->SetDragBorderVisible(false);
		}
	}

void ZUITextEngine::HandleDrop(ZPoint inPoint, ZDrop& inDrop)
	{
	if (!this->AcceptsDragDrop(inDrop))
		return;
	if (inDrop.GetDragSource() == this)
		{
		size_t dragOffset = this->PointToOffset(this->FromHost(inPoint));
		size_t selectStart, selectLength;
		this->GetSelection(selectStart, selectLength);

		if (dragOffset < selectStart)
			{
			string selectedText = this->GetText(selectStart, selectLength);
			this->DeleteText(selectStart, selectLength);
			this->InsertText(dragOffset, selectedText);
			this->SetSelection(dragOffset, selectLength);
			}
		else if (dragOffset > selectStart+selectLength)
			{
			string selectedText = this->GetText(selectStart, selectLength);
			this->DeleteText(selectStart, selectLength);
			this->InsertText(dragOffset-selectLength, selectedText);
			this->SetSelection(dragOffset-selectLength, selectLength);
			}				
		}
	else
		{
		size_t dragOffset = this->PointToOffset(inPoint);
		if (!this->IsRangeEditable(dragOffset, 0))
			return;
		for (size_t currentTupleIndex = 0; currentTupleIndex < inDrop.CountTuples(); ++currentTupleIndex)
			{
			ZTuple currentTuple = inDrop.GetTuple(currentTupleIndex);
			if (currentTuple.Has("text/plain"))
				{
				string droppedString = currentTuple.GetString("text/plain");
				this->InsertText(dragOffset, droppedString);
				this->SetSelection(dragOffset, droppedString.size());
				// Return now, so we only drop text from the first tuple that had any
				return;
				}
			}
		}
	}

void ZUITextEngine::SetDragOffset(size_t inDragOffset, bool inDragActive)
	{}

void ZUITextEngine::SetDragBorderVisible(bool inVisible)
	{}

void ZUITextEngine::SetupMenus(ZMenuSetup* inMenuSetup)
	{
	size_t selectStart, selectLength;
	this->GetSelection(selectStart, selectLength);

	bool notAllSelected = selectStart != 0 || selectLength != this->GetTextLength();
	inMenuSetup->EnableDisableMenuItem(mcSelectAll, notAllSelected);

	bool anySelected = selectLength != 0;
	inMenuSetup->EnableDisableMenuItem(mcCopy, anySelected);

	bool rangeEditable = this->IsRangeEditable(selectStart, selectLength);
	inMenuSetup->EnableDisableMenuItem(mcCut, anySelected && rangeEditable);
	inMenuSetup->EnableDisableMenuItem(mcClear, anySelected && rangeEditable);

	inMenuSetup->EnableDisableMenuItem(mcPaste, rangeEditable && ZClipboard::sGet().Has("text/plain"));
	}

bool ZUITextEngine::MenuMessage(const ZMessage& inMessage)
	{
	switch (inMessage.GetInt32("menuCommand"))
		{
		case mcCopy:
		case mcCut:
			{
			size_t selectStart, selectLength;
			this->GetSelection(selectStart, selectLength);
			if (selectLength != 0)
				{
				if (inMessage.GetInt32("menuCommand") == mcCopy || this->IsRangeEditable(selectStart, selectLength))
					{
					ZTuple theTuple = this->GetTuple(selectStart, selectLength);
					ZClipboard::sSet(theTuple);
					if (inMessage.GetInt32("menuCommand") == mcCut)
						this->DeleteText(selectStart, selectLength);
					}
				}
			return true;
			break;
			}
		case mcClear:
			{
			size_t selectStart, selectLength;
			this->GetSelection(selectStart, selectLength);
			if (selectLength != 0 && this->IsRangeEditable(selectStart, selectLength))
				{
				this->DeleteText(selectStart, selectLength);
				return true;
				}
			return true;
			break;
			}
		case mcPaste:
			{
			size_t selectStart, selectLength;
			this->GetSelection(selectStart, selectLength);
			if (this->IsRangeEditable(selectStart, selectLength))
				{
				ZTuple theTuple = ZClipboard::sGet();
				if (theTuple.Has("text/plain"))
					{
					this->ReplaceSelection(theTuple.GetString("text/plain"));
					return true;
					}
				}
			return true;
			break;
			}
		case mcSelectAll:
			{
			this->SetSelection(0, this->GetTextLength());
			return true;
			}
		}
	return false;
	}

ZDCRgn ZUITextEngine::GetTextRgn(size_t inOffset, size_t inLength)
	{
// Default implementation simply returns the rect of the entire text block
	return fHost->GetHostPane(this)->CalcBounds();
	}

ZTuple ZUITextEngine::GetTuple(size_t inOffset, size_t inLength)
	{
	ZTuple theTuple;
	theTuple.SetString("text/plain", this->GetText(inOffset, inLength));
	return theTuple;
	}

bool ZUITextEngine::IsRangeEditable(size_t inOffset, size_t inLength)
	{ return fHost->GetEngineEditable(this); }

ZPoint ZUITextEngine::FromHost(ZPoint inHostPoint)
	{ return inHostPoint - this->ToHost(ZPoint::sZero); }

ZPoint ZUITextEngine::ToHost(ZPoint inHostPoint)
	{ return inHostPoint + fHost->GetEngineLocation(this); }

// =================================================================================================
// =================================================================================================

ZUITextEngine::CheckScroll::CheckScroll(ZUITextEngine* inTextEngine)
:	fTextEngine(inTextEngine)
	{
	fOldThumbProportions = fTextEngine->GetScrollThumbProportions();
	fOldScrollValues = fTextEngine->GetScrollValues();
	}
ZUITextEngine::CheckScroll::~CheckScroll()
	{
	if (fOldThumbProportions == fTextEngine->GetScrollThumbProportions() && fOldScrollValues == fTextEngine->GetScrollValues())
		return;
	static_cast<ZUIScrollable*>(fTextEngine)->Internal_NotifyResponders();
// AG 99-07-05. Guess what? This can cause a deadlock -- indirectly at least, when using paige in Knowledge Forum there's
// a mess of mutexes. For the moment we'll pull this line (which isn't strictly necessary).
//	fTextEngine->GetHost()->GetHostPane(fTextEngine)->Update();
	}

// ==================================================

#define ASSERTLOCKED() ZAssertStop(1, this->GetLock().IsLocked());

ZUITextPane_TextEngine::ZUITextPane_TextEngine(ZSuperPane* superPane, ZPaneLocator* paneLocator, ZUITextEngine* inTextEngine)
:	ZUITextPane(superPane, paneLocator), fTextEngine(nil)
	{
	if (inTextEngine)
		this->SetTextEngine(inTextEngine);
	this->GetWindow()->RegisterIdlePane(this);
	}

ZUITextPane_TextEngine::~ZUITextPane_TextEngine()
	{
	this->SetTextEngine(nil);
	this->GetWindow()->UnregisterIdlePane(this);
	}

ZSubPane* ZUITextPane_TextEngine::GetHostPane(ZUITextEngine* inTextEngine)
	{
//	ASSERTLOCKED();
	return this;
	}

ZPoint ZUITextPane_TextEngine::GetEngineLocation(ZUITextEngine* inTextEngine)
	{
	ASSERTLOCKED();
	return ZPoint::sZero;
	}

void ZUITextPane_TextEngine::GetEngineSizes(ZUITextEngine* inTextEngine, ZPoint& outEngineSize, ZCoord& outEngineWidth)
	{
	ASSERTLOCKED();
	outEngineSize = this->GetSize();
// For the moment we'll force engines to wrap at our boundaries.
	outEngineWidth = outEngineSize.h;
	}

ZDCRgn ZUITextPane_TextEngine::GetEngineClip(ZUITextEngine* inTextEngine)
	{
	ASSERTLOCKED();
	return this->CalcVisibleBoundsRgn();
	}

bool ZUITextPane_TextEngine::GetEngineEditable(ZUITextEngine* inTextEngine)
	{
	ASSERTLOCKED();
	bool isEditable;
	if (this->GetAttribute("ZUITextPane::Editable", nil, &isEditable))
		return isEditable;
	return this->GetReallyEnabled();
	}

bool ZUITextPane_TextEngine::GetEngineActive(ZUITextEngine* inTextEngine)
	{
	ASSERTLOCKED();
	return this->IsWindowTarget() && this->GetActive();
	}

// Our protocol
ZUITextEngine* ZUITextPane_TextEngine::GetTextEngine()
	{
	ASSERTLOCKED();
	return fTextEngine;
	}

void ZUITextPane_TextEngine::SetTextEngine(ZUITextEngine* inTextEngine)
	{
	if (inTextEngine == fTextEngine)
		return;
	if (fTextEngine)
		{
		fTextEngine->Detached(this);
		static_cast<ZUISelection*>(fTextEngine)->RemoveResponder(this);
		static_cast<ZUIScrollable*>(fTextEngine)->RemoveResponder(this);
		delete fTextEngine;
		}
	fTextEngine = inTextEngine;
	if (fTextEngine)
		{
		static_cast<ZUISelection*>(fTextEngine)->AddResponder(this);
		static_cast<ZUIScrollable*>(fTextEngine)->AddResponder(this);
		fTextEngine->Attached(this);
		}
	}

// ==================================================

long ZUITextPane_TextEngine::GetUserChangeCount()
	{
	ASSERTLOCKED();
	return fTextEngine->GetUserChangeCount();
	}

// From ZUITextContainer by way of ZUITextPane
size_t ZUITextPane_TextEngine::GetTextLength()
	{
	ASSERTLOCKED();
	return fTextEngine->GetTextLength();
	}

void ZUITextPane_TextEngine::GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength)
	{
	ASSERTLOCKED();
	fTextEngine->GetText(outDest, inOffset, inLength, outLength);
	}

void ZUITextPane_TextEngine::SetText_Impl(const char* inSource, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->SetText(inSource, inLength);
	}

void ZUITextPane_TextEngine::InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->InsertText(inInsertOffset, inSource, inLength);
	}

void ZUITextPane_TextEngine::DeleteText(size_t inOffset, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->DeleteText(inOffset, inLength);
	}

void ZUITextPane_TextEngine::ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->ReplaceText(inReplaceOffset, inReplaceLength, inSource, inLength);
	}

void ZUITextPane_TextEngine::GetSelection(size_t& outOffset, size_t& outLength)
	{
	ASSERTLOCKED();
	fTextEngine->GetSelection(outOffset, outLength);
	}

void ZUITextPane_TextEngine::SetSelection(size_t inOffset, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->SetSelection(inOffset, inLength);
	}

void ZUITextPane_TextEngine::ReplaceSelection_Impl(const char* inSource, size_t inLength)
	{
	ASSERTLOCKED();
	fTextEngine->ReplaceSelection(inSource, inLength);
	}

// From ZUIScrollable::Responder
void ZUITextPane_TextEngine::ScrollableChanged(ZUIScrollable* inScrollable)
	{
	ASSERTLOCKED();
// For now the only scrollable we're connected to is our text engine
	ZAssert(inScrollable == fTextEngine);
	static_cast<ZUIScrollable*>(this)->Internal_NotifyResponders();
	}

// From ZUISelection::Responder
void ZUITextPane_TextEngine::HandleUISelectionEvent(ZUISelection* inSelection, ZUISelection::Event inEvent)
	{
	ASSERTLOCKED();
// For now the only selection we're connected to is our text engine
	ZAssert(inSelection == fTextEngine);
	static_cast<ZUISelection*>(this)->Internal_NotifyResponders(inEvent);
	}

// From ZUIScrollable by way of ZTUTextPane
void ZUITextPane_TextEngine::ScrollBy(ZPoint_T<double> delta, bool inDrawIt, bool inDoNotifications)
	{
	ASSERTLOCKED();
	fTextEngine->ScrollBy(delta, inDrawIt, inDoNotifications);
	}

void ZUITextPane_TextEngine::ScrollTo(ZPoint_T<double> newValues, bool inDrawIt, bool inDoNotifications)
	{
	ASSERTLOCKED();
	fTextEngine->ScrollTo(newValues, inDrawIt, inDoNotifications);
	}

void ZUITextPane_TextEngine::ScrollStep(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications)
	{
	ASSERTLOCKED();
	fTextEngine->ScrollStep(increase, horizontal, inDrawIt, inDoNotifications);
	}

void ZUITextPane_TextEngine::ScrollPage(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications)
	{
	ASSERTLOCKED();
	fTextEngine->ScrollPage(increase, horizontal, inDrawIt, inDoNotifications);
	}

ZPoint_T<double> ZUITextPane_TextEngine::GetScrollValues()
	{
	ASSERTLOCKED();
	return fTextEngine->GetScrollValues();
	}

ZPoint_T<double> ZUITextPane_TextEngine::GetScrollThumbProportions()
	{
	ASSERTLOCKED();
	return fTextEngine->GetScrollThumbProportions();
	}

void ZUITextPane_TextEngine::SetFont(const ZDCFont& inFont)
	{
	ASSERTLOCKED();
	fTextEngine->SetFont(inFont);
	}

void ZUITextPane_TextEngine::HandleDrag(ZSubPane* inSubPane, ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag)
	{
	ASSERTLOCKED();
	ZAssertStop(2, inSubPane == this);
	fTextEngine->HandleDrag(inPoint, inMotion, inDrag);
	}

void ZUITextPane_TextEngine::HandleDrop(ZSubPane* inSubPane, ZPoint inPoint, ZDrop& inDrop)
	{
	ASSERTLOCKED();
	ZAssertStop(2, inSubPane == this);
	fTextEngine->HandleDrop(inPoint, inDrop);
	this->BecomeWindowTarget();
	}

void ZUITextPane_TextEngine::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ASSERTLOCKED();
#if 1
	ZSubPane::DoDraw(inDC, inBoundsRgn);
	fTextEngine->Draw(inDC);
#else
	ZDC_OffAuto localDC(inDC, false);
	ZSubPane::DoDraw(localDC);
	fTextEngine->Draw(localDC);
#endif
	}

bool ZUITextPane_TextEngine::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ASSERTLOCKED();
	return fTextEngine->Click(inHitPoint, inEvent);
	}

bool ZUITextPane_TextEngine::DoKeyDown(const ZEvent_Key& inEvent)
	{
	ASSERTLOCKED();
	return fTextEngine->KeyDown(inEvent);
	}

bool ZUITextPane_TextEngine::GetCursor(ZPoint inPoint, ZCursor& outCursor)
	{
	ASSERTLOCKED();
	if (this->GetReallyEnabled() && this->GetActive())
		{
		if (fTextEngine->GetCursor(inPoint, outCursor))
			return true;
		}
	return ZUITextPane::GetCursor(inPoint, outCursor);
	}

void ZUITextPane_TextEngine::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	ASSERTLOCKED();
	if (this->GetReallyEnabled() && this->GetActive())
		fTextEngine->MouseMotion(inPoint, inMotion);
	ZUITextPane::MouseMotion(inPoint, inMotion);
	}

ZDragDropHandler* ZUITextPane_TextEngine::GetDragDropHandler()
	{
	ASSERTLOCKED();
	ZDragDropHandler* theDragDropHandler = ZSubPane::GetDragDropHandler();
	if (theDragDropHandler != nil)
		return theDragDropHandler;
	return this;
	}

void ZUITextPane_TextEngine::Activate(bool entering)
	{
	ASSERTLOCKED();
	if (this->IsWindowTarget())
		fTextEngine->Activate(entering);
	}

void ZUITextPane_TextEngine::DoIdle()
	{
	ASSERTLOCKED();
	ZDC theDC = this->GetDC();
	fTextEngine->Idle(theDC, this->GetEngineActive(fTextEngine) && this->GetEngineEditable(fTextEngine));
	theDC.Flush();
	}

void ZUITextPane_TextEngine::DoSetupMenus(ZMenuSetup* inMenuSetup)
	{
	ASSERTLOCKED();
	fTextEngine->SetupMenus(inMenuSetup);
	}

bool ZUITextPane_TextEngine::DoMenuMessage(const ZMessage& inMessage)
	{
	ASSERTLOCKED();
	if (fTextEngine->MenuMessage(inMessage))
		return true;
	return ZUITextPane::DoMenuMessage(inMessage);
	}

void ZUITextPane_TextEngine::BecomeTabTarget(bool inTabbingBackwards)
	{
	ASSERTLOCKED();
	fTextEngine->SetSelection(0, fTextEngine->GetTextLength());
	ZSubPane::BecomeTabTarget(inTabbingBackwards);
	}

bool ZUITextPane_TextEngine::WantsToBecomeTarget()
	{
	ASSERTLOCKED();
	return true;
	}

void ZUITextPane_TextEngine::HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	ASSERTLOCKED();
	ZSubPane::HandlerBecameWindowTarget(inOldTarget, inNewTarget);
	if (fTextEngine && this->GetActive())
		fTextEngine->Activate(true);
	}

void ZUITextPane_TextEngine::HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	ASSERTLOCKED();
	ZSubPane::HandlerResignedWindowTarget(inOldTarget, inNewTarget);
	if (fTextEngine && this->GetActive())
		fTextEngine->Activate(false);
	}

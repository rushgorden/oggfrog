static const char rcsid[] = "@(#) $Id: ZEventHr.cpp,v 1.2 2006/07/12 19:41:08 agreen Exp $";

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

#include "ZEventHr.h"

#include "ZDebug.h"
#include "ZMemory.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHr

ZEventHr::ZEventHr(ZEventHr* inParentHandler)
:	fParentHandler(inParentHandler), fFilter(nil)
	{}

ZEventHr::~ZEventHr()
	{
	if (fFilter)
		fFilter->FilterEventHrBeingFreed(this);
	if (fParentHandler)
		fParentHandler->SubHandlerBeingFreed(this);
	}

void ZEventHr::SubHandlerBeingFreed(ZEventHr* inHandler)
	{
	if (fParentHandler)
		fParentHandler->SubHandlerBeingFreed(inHandler);
	}

ZEventHrFilter* ZEventHr::GetEventFilter()
	{ return fFilter; }

ZEventHrFilter* ZEventHr::SetEventFilter(ZEventHrFilter* inFilter)
	{
	ZEventHrFilter* oldFilter = fFilter;

	if (oldFilter)
		oldFilter->FilterRemoved(this);

	fFilter = inFilter;

	if (fFilter)
		fFilter->FilterInstalled(this);

	return oldFilter;
	}

void ZEventHr::HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	if (fFilter)
		fFilter->FilterBecameWindowTarget(this, inOldTarget, inNewTarget);

	if (fParentHandler)
		fParentHandler->HandlerBecameWindowTarget(inOldTarget, inNewTarget);
	}
void ZEventHr::HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	if (fFilter)
		fFilter->FilterResignedWindowTarget(this, inOldTarget, inNewTarget);

	if (fParentHandler)
		fParentHandler->HandlerResignedWindowTarget(inOldTarget, inNewTarget);
	}

ZEventHr* ZEventHr::GetWindowTarget()
	{
	if (fParentHandler)
		return fParentHandler->GetWindowTarget();
	return nil;
	}

void ZEventHr::BecomeWindowTarget()
	{ this->HandlerBecomeWindowTarget(this); }

void ZEventHr::ResignWindowTarget()
	{ this->HandlerResignWindowTarget(this); }


void ZEventHr::HandlerBecomeWindowTarget(ZEventHr* inHandler)
	{
// We've got to have a next handler *or* we should override this method
	ZAssert(fParentHandler);
	if (fParentHandler)
		fParentHandler->HandlerBecomeWindowTarget(inHandler);
	}

bool ZEventHr::IsParentOf(ZEventHr* possibleChild)
	{
	ZEventHr* theHandler = possibleChild;
	while (theHandler != nil)
		{
		if (theHandler == this)
			return true;
		theHandler = theHandler->GetParentHandler();
		}
	return false;
	}

void ZEventHr::HandlerResignWindowTarget(ZEventHr* inHandler)
	{
	if (inHandler != this && this->WantsToBecomeTarget())
		this->HandlerBecomeWindowTarget(this);
	else
		{
		if (fParentHandler)
			fParentHandler->HandlerResignWindowTarget(inHandler);
		else
			ZDebugStopf(1,
				("ZEventHr::HandlerResignWindowTarget, couldn't find a handler to resign to"));
		}
	}

bool ZEventHr::DispatchAutoKey(const ZEvent_Key& inEvent)
	{
	if (this->HandleAutoKey(inEvent, this, true))
		return true;
	if (this->HandleAutoKey(inEvent, this, false))
		return true;
	return this->DispatchKeyDown(inEvent);
	}

bool ZEventHr::HandleAutoKey(const ZEvent_Key& inEvent,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fParentHandler && fParentHandler->HandleAutoKey(inEvent, inOriginalTarget, true))
			return true;
		if (fFilter && fFilter->FilterAutoKey(this, inEvent, inOriginalTarget, true))
			return true;
		return false;
		}

	if (fFilter && fFilter->FilterAutoKey(this, inEvent, inOriginalTarget, false))
		return true;

	if (this->DoAutoKey(inEvent))
		return true;

	if (fParentHandler && fParentHandler->HandleAutoKey(inEvent, inOriginalTarget, false))
		return true;

	return false;
	}

bool ZEventHr::DoAutoKey(const ZEvent_Key& inEvent)
	{ return false; }

bool ZEventHr::DispatchKeyDown(const ZEvent_Key& inEvent)
	{
	if (this->HandleKeyDown(inEvent, this, true))
		return true;

	return this->HandleKeyDown(inEvent, this, false);
	}

bool ZEventHr::HandleKeyDown(const ZEvent_Key& inEvent,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fParentHandler && fParentHandler->HandleKeyDown(inEvent, inOriginalTarget, true))
			return true;
		if (fFilter && fFilter->FilterKeyDown(this, inEvent, inOriginalTarget, true))
			return true;
		return false;
		}

	if (fFilter && fFilter->FilterKeyDown(this, inEvent, inOriginalTarget, false))
		return true;

	if (this->DoKeyDown(inEvent))
		return true;

	if (fParentHandler && fParentHandler->HandleKeyDown(inEvent, inOriginalTarget, false))
		return true;

	return false;
	}

bool ZEventHr::DoKeyDown(const ZEvent_Key& inEvent)
	{ return false; }

void ZEventHr::DispatchInstallMenus(ZMenuInstall& inMenuInstall)
	{
	this->HandleInstallMenus(inMenuInstall, this, true);
	this->HandleInstallMenus(inMenuInstall, this, false);
	}

void ZEventHr::HandleInstallMenus(ZMenuInstall& inMenuInstall,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fParentHandler)
			fParentHandler->HandleInstallMenus(inMenuInstall, inOriginalTarget, true);
		this->DoInstallMenus(&inMenuInstall);
		return;
		}

	if (fParentHandler)
		fParentHandler->HandleInstallMenus(inMenuInstall, inOriginalTarget, false);

	if (fFilter)
		fFilter->FilterInstallMenus(this, inMenuInstall);
	}

void ZEventHr::DoInstallMenus(ZMenuInstall* inMenuInstall)
	{}

void ZEventHr::DispatchSetupMenus(ZMenuSetup& inMenuSetup)
	{
	this->HandleSetupMenus(inMenuSetup, this, true);
	this->HandleSetupMenus(inMenuSetup, this, false);
	}

void ZEventHr::HandleSetupMenus(ZMenuSetup& inMenuSetup,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fParentHandler)
			fParentHandler->HandleSetupMenus(inMenuSetup, inOriginalTarget, true);
		this->DoSetupMenus(&inMenuSetup);
		return;
		}

	if (fParentHandler)
		fParentHandler->HandleSetupMenus(inMenuSetup, inOriginalTarget, false);

	if (fFilter)
		fFilter->FilterSetupMenus(this, inMenuSetup);
	}

void ZEventHr::DoSetupMenus(ZMenuSetup* inMenuSetup)
	{}

bool ZEventHr::DispatchMenuMessage(const ZMessage& inMessage)
	{
	if (this->HandleMenuMessage(inMessage, this, true))
		return true;

	return this->HandleMenuMessage(inMessage, this, false);
	}

bool ZEventHr::HandleMenuMessage(const ZMessage& inMessage,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fParentHandler)
			fParentHandler->HandleMenuMessage(inMessage, inOriginalTarget, true);
		return false;
		}

	if (fFilter && fFilter->FilterMenuMessage(this, inMessage))
		return true;

	if (this->DoMenuMessage(inMessage))
		return true;

	if (fParentHandler && fParentHandler->HandleMenuMessage(inMessage, inOriginalTarget, false))
		return true;

	return false;
	}

bool ZEventHr::DoMenuMessage(const ZMessage& inMessage)
	{
	// Call our backwards compatibility version
	return this->DoMenuCommand(inMessage.GetInt32("menuCommand"));
	}

bool ZEventHr::DoMenuCommand(int32 inMenuCommand)
	{
	return false;
	}

bool ZEventHr::WantsToBecomeTarget()
	{ return false; }

void ZEventHr::InsertTabTargets(vector<ZEventHr*>& inOutTargets)
	{
	if (this->WantsToBecomeTabTarget())
		inOutTargets.push_back(this);
	}

ZEventHr* ZEventHr::FindTabTarget(ZEventHr* inCurrentEventHandler, bool inTabbingForwards)
	{
	ZEventHr* newEventHandler = inCurrentEventHandler;
	// We walk our tree depth first, which will more or less
	// correspond to what the user expects to see
	vector<ZEventHr*> theVector;
	this->InsertTabTargets(theVector);

	if (theVector.size() != 0)
		{
		// Find the current target
		vector<ZEventHr*>::iterator currentIter = theVector.end();
		if (inCurrentEventHandler)
			currentIter = find(theVector.begin(), theVector.end(), inCurrentEventHandler);

		if (inTabbingForwards)
			{
			if (currentIter == theVector.end())
				{
				// We didn't find the current event handler, go to the beginning.
				currentIter = theVector.begin();
				}
			else
				{
				// Otherwise, click on one, and wrap if we reach the end
				++currentIter;
				if (currentIter == theVector.end())
					currentIter = theVector.begin();
				}
			}
		else
			{
			// If we're at the beginning, or we didn't find the current event handler,
			// then go to the end of the list
			if (currentIter == theVector.end() || currentIter == theVector.begin())
				currentIter = theVector.end();
			--currentIter;
			}
		ZAssert(currentIter != theVector.end());
		newEventHandler = *currentIter;
		}
	return newEventHandler;
	}

ZEventHr* ZEventHr::GetParentHandler()
	{ return fParentHandler; }

void ZEventHr::BecomeTabTarget(bool inTabbingForwards)
	{
	// Right now, only overridden by text editing pane, which selects all its text.
	// Its normal become target stuff just switches on the text hiliting
	this->BecomeWindowTarget();
	}

bool ZEventHr::WantsToBecomeTabTarget()
	{
	// Most event handlers do *not* want to become the target because
	// of tabbing (only panes are gonna want to do this)
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHrFilter

void ZEventHrFilter::FilterEventHrBeingFreed(ZEventHr* inFilteredEventHr)
	{}

void ZEventHrFilter::FilterInstalled(ZEventHr* inFilteredEventHr)
	{}

void ZEventHrFilter::FilterRemoved(ZEventHr* inFilteredEventHr)
	{}

bool ZEventHrFilter::FilterKeyDown(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{ return false; }

bool ZEventHrFilter::FilterAutoKey(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent,
	ZEventHr* inOriginalTarget, bool inFirstCall)
	{ return false; }

void ZEventHrFilter::FilterInstallMenus(ZEventHr* inFilteredEventHr, ZMenuInstall& inMenuInstall)
	{}

void ZEventHrFilter::FilterSetupMenus(ZEventHr* inFilteredEventHr, ZMenuSetup& inMenuSetup)
	{}

bool ZEventHrFilter::FilterMenuMessage(ZEventHr* inFilteredEventHr, const ZMessage& inMessage)
	{ return false; }

void ZEventHrFilter::FilterBecameWindowTarget(ZEventHr* inFilteredEventHr,
	ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{}

void ZEventHrFilter::FilterResignedWindowTarget(ZEventHr* inFilteredEventHr,
	ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHrFilter_TargetTab

ZEventHrFilter_TargetTab::ZEventHrFilter_TargetTab(ZEventHr* inEventHr)
:	fEventHr(inEventHr)
	{
	ZAssert(fEventHr);
	ZEventHrFilter* oldFilter = fEventHr->SetEventFilter(this);
	// We don't do event filter chaining (yet)
	ZAssert(oldFilter == nil);
	}

void ZEventHrFilter_TargetTab::FilterEventHrBeingFreed(ZEventHr* inEventHr)
	{
	ZAssert(inEventHr == fEventHr);
	delete this;
	}

bool ZEventHrFilter_TargetTab::FilterKeyDown(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent, ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	// If we do our work when inFirstCall is true, then we'll absorb every tab character before it ever gets dispatched to any
	// child event handler's DoKeyDown method
	if (inFirstCall)
		{
		if (inEvent.GetCharCode() == ZKeyboard::ccTab)
			{
			ZEventHr* newEventHr = fEventHr->FindTabTarget(inOriginalTarget, !inEvent.GetShift());
			if (newEventHr)
				newEventHr->BecomeTabTarget(!inEvent.GetShift());
			return true;
			}
		}
	return ZEventHrFilter::FilterKeyDown(inFilteredEventHr, inEvent, inOriginalTarget, inFirstCall);
	}

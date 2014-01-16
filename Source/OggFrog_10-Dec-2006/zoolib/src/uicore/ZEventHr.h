/*  @(#) $Id: ZEventHr.h,v 1.4 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZEventHr__
#define __ZEventHr__ 1
#include "zconfig.h"

#include "ZEvent.h"
#include "ZMenu.h"

class ZEventHrFilter;

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHr

class ZEventHr
	{
public:
	ZEventHr(ZEventHr* inParentHandler);
	virtual ~ZEventHr();

// Event filters
	ZEventHrFilter* GetEventFilter();
	ZEventHrFilter* SetEventFilter(ZEventHrFilter* inFilter);

// Key strokes
	virtual bool DispatchAutoKey(const ZEvent_Key& inEvent);
	virtual bool HandleAutoKey(const ZEvent_Key& inEvent,
		ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual bool DoAutoKey(const ZEvent_Key& inEvent);

	virtual bool DispatchKeyDown(const ZEvent_Key& inEvent);
	virtual bool HandleKeyDown(const ZEvent_Key& inEvent,
		ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);

// Menu handling
	virtual void DispatchInstallMenus(ZMenuInstall& inMenuInstall);
	virtual void HandleInstallMenus(ZMenuInstall& inMenuInstall,
		ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual void DoInstallMenus(ZMenuInstall* inMenuInstall);

	virtual void DispatchSetupMenus(ZMenuSetup& inMenuSetup);
	virtual void HandleSetupMenus(ZMenuSetup& inMenuSetup,
		ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual void DoSetupMenus(ZMenuSetup* inMenuSetup);

	virtual bool DispatchMenuMessage(const ZMessage& inMessage);
	virtual bool HandleMenuMessage(const ZMessage& inMessage,
		ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual bool DoMenuMessage(const ZMessage& inMessage);

// Backwards compatibility
	virtual bool DoMenuCommand(int32 inMenuCommand);

// Event handler chain
	virtual void SubHandlerBeingFreed(ZEventHr* inHandler);
	ZEventHr* GetParentHandler();

	bool IsParentOf(ZEventHr* possibleChild);

	virtual bool WantsToBecomeTarget();

	void BecomeWindowTarget();
	void ResignWindowTarget();

	virtual ZEventHr* GetWindowTarget();

	virtual void HandlerBecomeWindowTarget(ZEventHr* inHandler);
	virtual void HandlerResignWindowTarget(ZEventHr* inHandler);

// And notifications of handler changes
	virtual void HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual void HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);

// Tabbing support
	virtual bool WantsToBecomeTabTarget();
	virtual void InsertTabTargets(vector<ZEventHr*>& inOutTargets);
	virtual ZEventHr* FindTabTarget(ZEventHr* inCurrentEventHandler, bool inTabbingForwards);
	virtual void BecomeTabTarget(bool inTabbingForwards);

protected:
	ZEventHr* fParentHandler;
	ZEventHrFilter* fFilter;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHrFilter

class ZEventHrFilter
	{
public:
	virtual void FilterEventHrBeingFreed(ZEventHr* inFilteredEventHr);

	virtual void FilterInstalled(ZEventHr* inFilteredEventHr);
	virtual void FilterRemoved(ZEventHr* inFilteredEventHr);

	virtual bool FilterKeyDown(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent,
		ZEventHr* inOriginalTarget, bool inFirstCall);

	virtual bool FilterAutoKey(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent,
		ZEventHr* inOriginalTarget, bool inFirstCall);

	virtual void FilterInstallMenus(ZEventHr* inFilteredEventHr, ZMenuInstall& inMenuInstall);

	virtual void FilterSetupMenus(ZEventHr* inFilteredEventHr, ZMenuSetup& inMenuSetup);

	virtual bool FilterMenuMessage(ZEventHr* inFilteredEventHr, const ZMessage& inMessage);

	virtual void FilterBecameWindowTarget(ZEventHr* inFilteredEventHr,
		ZEventHr* inOldTarget, ZEventHr* inNewTarget);

	virtual void FilterResignedWindowTarget(ZEventHr* inFilteredEventHr,
		ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZEventHrFilter_TargetTab

// A much-used filter, if not actually essential

class ZEventHrFilter_TargetTab : public ZEventHrFilter
	{
public:
	ZEventHrFilter_TargetTab(ZEventHr* inEventHr);

	virtual void FilterEventHrBeingFreed(ZEventHr* inEventHr);
	virtual bool FilterKeyDown(ZEventHr* inFilteredEventHr, const ZEvent_Key& inEvent, ZEventHr* inOriginalTarget, bool inFirstCall);

private:
	ZEventHr* fEventHr;
	};

#endif // __ZEventHr__

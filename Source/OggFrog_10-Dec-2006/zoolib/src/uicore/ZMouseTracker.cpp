static const char rcsid[] = "@(#) $Id: ZMouseTracker.cpp,v 1.3 2006/04/10 20:44:22 agreen Exp $";

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
#include "ZMouseTracker.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZMouseTracker

ZMouseTracker::ZMouseTracker(ZSubPane* inPane, ZPoint inStartPoint)
:	ZCaptureInput(inPane),
	fAnchor(inStartPoint),
	fPrevious(inStartPoint)
	{}

ZMouseTracker::~ZMouseTracker()
	{
	}

bool ZMouseTracker::HandleMessage(const ZMessage& inMessage)
	{
	string theWhat = inMessage.GetString("what");
	if (theWhat == "zoolib:MouseDown")
		{
		if (fPane)
			this->ButtonDown(fPane->FromWindow(inMessage.GetPoint("where")), ZMouse::eButtonLeft);
		return true;
		}
	else if (theWhat == "zoolib:MouseUp")
		{
		if (fPane)
			this->ButtonUp(fPane->FromWindow(inMessage.GetPoint("where")), ZMouse::eButtonLeft);
		return true;
		}
	else if (theWhat == "zoolib:MouseEnter" || theWhat == "zoolib:MouseChange" || theWhat == "zoolib:MouseExit")
		{
		if (fPane)
			this->Moved(fPane->FromWindow(inMessage.GetPoint("where")));
		return true;
		}
	return false;
	}

void ZMouseTracker::ButtonDown(ZPoint inPanePoint, ZMouse::Button inButton)
	{
	}

void ZMouseTracker::ButtonUp(ZPoint inPanePoint, ZMouse::Button inButton)
	{
	this->Finish();
	}

void ZMouseTracker::Moved(ZPoint inPanePoint)
	{
	fPane->BringPointIntoViewPinned(inPanePoint);
	this->TrackIt(ePhaseContinue, fAnchor, fPrevious, inPanePoint, fPrevious != inPanePoint);
	fPrevious = inPanePoint;
	}

void ZMouseTracker::Lingered(ZPoint inPanePoint)
	{
	fPane->BringPointIntoViewPinned(inPanePoint);
	this->TrackIt(ePhaseContinue, fAnchor, fPrevious, inPanePoint, fPrevious != inPanePoint);
	fPrevious = inPanePoint;
	}

void ZMouseTracker::Installed()
	{
	this->TrackIt(ePhaseBegin, fAnchor, fPrevious, fPrevious, false);
	}

void ZMouseTracker::Removed()
	{
	this->TrackIt(ePhaseEnd, fAnchor, fPrevious, fPrevious, false);
	fPane = nil;
	}

void ZMouseTracker::Cancelled()
	{
	this->TrackIt(ePhaseEnd, fAnchor, fPrevious, fPrevious, false);
	fPane = nil;
	}

void ZMouseTracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{}


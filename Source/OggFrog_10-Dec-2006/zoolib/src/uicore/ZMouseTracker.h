/*  @(#) $Id: ZMouseTracker.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZMouseTracker__
#define __ZMouseTracker__ 1
#include "zconfig.h"

#include "ZFakeWindow.h" // For ZCaptureInput

// =================================================================================================
#pragma mark -
#pragma mark * ZMouseTracker

class ZMouseTracker : public ZCaptureInput
	{
public:
	ZMouseTracker(ZSubPane* inPane, ZPoint inStartPoint);
	virtual ~ZMouseTracker();

// New API
	virtual bool HandleMessage(const ZMessage& inMessage);

	virtual void ButtonDown(ZPoint inPanePoint, ZMouse::Button inButton);
	virtual void ButtonUp(ZPoint inPanePoint, ZMouse::Button inButton);
	virtual void Moved(ZPoint inPanePoint);
	virtual void Lingered(ZPoint inPanePoint);

	virtual void Installed();
	virtual void Removed();
	virtual void Cancelled();

// Backwards compatible API
	enum Phase
		{
		ePhaseBegin,
		ePhaseContinue,
		ePhaseEnd
		};

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

private:
	ZPoint fAnchor;
	ZPoint fPrevious;
	};

#endif // __ZMouseTracker__

/*  @(#) $Id: ZUITextEngine_Mac.h,v 1.3 2002/07/12 00:31:54 agreen Exp $ */

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

#ifndef __ZUITextEngine_Mac__
#define __ZUITextEngine_Mac__ 1
#include "zconfig.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

#include "ZUITextEngine.h"

class ZUITextEngine_TextEdit : public ZUITextEngine
	{
public:
	ZUITextEngine_TextEdit(const ZDCFont& theFont);
	virtual ~ZUITextEngine_TextEdit();

// From ZUITextEngine
	virtual void Draw(const ZDC& inDC);
	virtual void DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength);
	virtual bool Click(ZPoint theHitPoint, const ZEvent_Mouse& theEvent);
	virtual bool KeyDown(const ZEvent_Key& theEvent);
	virtual void Activate(bool entering);
	virtual void Idle(const ZDC& inDC, bool inEnabledAndActive);

	virtual long GetUserChangeCount();

	virtual void SetFont(const ZDCFont& theFont);

	virtual void SetTextColor(ZRGBColor textColor);
	virtual ZRGBColor GetTextColor();
	virtual void SetBackColor(ZRGBColor backColor);
	virtual ZRGBColor GetBackColor();

	virtual size_t PointToOffset(ZPoint inPoint);

	virtual ZDCRgn GetTextRgn(size_t inOffset, size_t inLength);

	virtual void SetDragOffset(size_t inDragOffset, bool inDragActive);
	virtual void SetDragBorderVisible(bool inVisible);

// From ZUITextContainer via ZUITextEngine
	virtual size_t GetTextLength();

	virtual void GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength);

	virtual void SetText_Impl(const char* inSource, size_t inLength);

	virtual void InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength);

	virtual void DeleteText(size_t inOffset, size_t inLength);

	virtual void ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength);

	virtual void GetSelection(size_t& outOffset, size_t& outLength);
	virtual void SetSelection(size_t inOffset, size_t inLength);
	virtual void ReplaceSelection_Impl(const char* inSource, size_t inLength);

// From ZUIScrollable via ZUITextEngine
	virtual void ScrollBy(ZPoint_T<double> delta, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollTo(ZPoint_T<double> newValues, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollStep(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollPage(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications);

	virtual ZPoint_T<double> GetScrollValues();
	virtual ZPoint_T<double> GetScrollThumbProportions();
// Our protocol
	ZDCFont GetFont();

protected:
	void Internal_ScrollTo(ZPoint translation, bool inDrawIt, bool inDoNotifications);
	void Internal_ToggleDragCaret(const ZDC& inDC, size_t inDragOffset);
	void Internal_ToggleDragBorder(const ZDC& inDC);
	bool Internal_CheckGeometry();
	void UpdateTECalc();
#if ZCONFIG(OS, MacOS7)
	bool TEClickLoop();
	static pascal Boolean sTEClickLoop(TEPtr inTEPtr);
	static TEClickLoopUPP sTEClickLoopUPP;
	static ZUITextEngine_TextEdit* sCurrentTextEngine;
#endif // ZCONFIG(OS, MacOS7)

	class BeInPort;
	friend class BeInPort;

	TEHandle fTEHandle;
	ZRGBColor fTextColor;
	ZRGBColor fBackColor;

	long fChangeCounter;
	long fLastKnownChangeCounter;
	size_t fLastKnownSelectionOffset;
	size_t fLastKnownSelectionLength;

	size_t fDragOffset;
	bool fDragActive;
	bool fDragBorderVisible;
	};


#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)


#endif // __ZUITextEngine_Mac__

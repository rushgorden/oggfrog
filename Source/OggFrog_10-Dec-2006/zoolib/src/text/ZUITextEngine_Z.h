/*  @(#) $Id: ZUITextEngine_Z.h,v 1.3 2002/07/12 00:31:54 agreen Exp $ */

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

#ifndef __ZUITextEngine_Z__
#define __ZUITextEngine_Z__ 1
#include "zconfig.h"

#include "ZUITextEngine.h"

/* AG 1999-09-15. This is really just a stopgap. It is an experimental piece to explore the implementation of
text editing using only zlib facilities. It is useful right now for its ability to do password text editing, where
each normal glyph is replaced by a bullet character.
*/


class ZUITextEngine_Z : public ZUITextEngine
	{
public:
	ZUITextEngine_Z(const ZDCFont& inFont, char inBulletChar);
	ZUITextEngine_Z(const ZDCFont& inFont);
	void Internal_Initialize();

	virtual ~ZUITextEngine_Z();

// From ZUITextEngine itself
	virtual void Draw(const ZDC& inDC);
	virtual void DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength);
	virtual bool Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool KeyDown(const ZEvent_Key& inEvent);
	virtual void MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion);
	virtual void Activate(bool entering);
	virtual void Idle(const ZDC& inDC, bool inEnabledAndActive);

	virtual void SetDragOffset(size_t inDragOffset, bool inDragActive);
	virtual void SetDragBorderVisible(bool inVisible);

	virtual void SetFont(const ZDCFont& inFont) ;

	virtual void SetTextColor(ZRGBColor inTextColor);
	virtual ZRGBColor GetTextColor();
	virtual void SetBackColor(ZRGBColor inBackColor);
	virtual ZRGBColor GetBackColor();

	virtual size_t PointToOffset(ZPoint inPoint);

	virtual ZDCRgn GetTextRgn(size_t inOffset, size_t inLength);

	virtual void HandleDrag(ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag);
	virtual void HandleDrop(ZPoint inPoint, ZDrop& inDrop);
	virtual bool AcceptsDragDrop(const ZDragDropBase& inDragDrop);

	virtual ZTuple GetTuple(size_t inOffset, size_t inLength);

	virtual long GetUserChangeCount();

	virtual bool IsRangeEditable(size_t inOffset, size_t inLength);

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
	virtual void ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);

	virtual ZPoint_T<double> GetScrollValues();
	virtual ZPoint_T<double> GetScrollThumbProportions();
// Our protocol
	ZDCFont GetFont();

protected:
	void Internal_Invalidate();
	void Internal_GetSelection(size_t& outSelectPrimary, size_t& outSelectSecondary);
	void Internal_SetSelection(size_t inSelectPrimary, size_t inSelectSecondary);
	void Internal_ToggleDrawCaretAt(const ZDC& inDC, size_t inCaretOffset);
	ZPoint Internal_OffsetToPoint(size_t inOffset);
	void Internal_ToggleDragCaret(const ZDC& inDC, size_t inDragOffset);
	void Internal_ToggleDragBorder(const ZDC& inDC);

	string fText;
	ZDCFont fFont;
	ZRGBColor fColor;
	bool fPassword;
	char fBulletChar;

	size_t fSelectPrimary;
	size_t fSelectSecondary;

	bool fActive;
	bigtime_t fLastCaretFlash;
	bool fCaretShown;

	size_t fDragOffset;
	bool fDragActive;
	bool fDragBorderVisible;

	long fChangeCounter;
	long fLastKnownChangeCounter;
	size_t fLastKnownSelectPrimary;
	size_t fLastKnownSelectSecondary;

	class Tracker;
	friend class Tracker;
	};


#endif // __ZUITextEngine_Z__

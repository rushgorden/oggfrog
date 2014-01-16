static const char rcsid[] = "@(#) $Id: ZUITextEngine_Z.cpp,v 1.6 2006/07/12 19:35:40 agreen Exp $";

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

#include "ZUITextEngine_Z.h"
#include "ZMouseTracker.h"
#include "ZUtil_Graphics.h"
#include "ZMemory.h"
#include "ZTicks.h"
#include "ZUIFactory.h"

static void sBreakLine(ZDC& inDC, bool inPassword, char inBulletChar, const char* inLineStart, size_t inRemainingTextLength, ZCoord inTargetWidth, size_t& outNextLineDelta)
	{
	if (inRemainingTextLength == 0)
		{
		outNextLineDelta = 0;
		return;
		}

	if (inTargetWidth < 0)
		inTargetWidth = 0;

	if (inPassword)
		{
		string fakeString(inRemainingTextLength, inBulletChar);
		inDC.BreakLine(fakeString.data(), inRemainingTextLength, inTargetWidth, outNextLineDelta);
		}
	else
		{
		inDC.BreakLine(inLineStart, inRemainingTextLength, inTargetWidth, outNextLineDelta);
		}
	}

static void sDrawText(ZDC& inDC, bool inPassword, char inBulletChar, const ZPoint& inLocation, const char* inText, size_t inTextLength)
	{
	if (inPassword)
		{
		string fakeString(inTextLength, inBulletChar);
		inDC.DrawText(inLocation, fakeString.data(), inTextLength);
		}
	else
		{
		inDC.DrawText(inLocation, inText, inTextLength);
		}
	}

static ZCoord sGetTextWidth(ZDC& inDC, bool inPassword, char inBulletChar, const char* inText, size_t inTextLength)
	{
	if (inPassword)
		{
		string fakeString(inTextLength, inBulletChar);
		return inDC.GetTextWidth(fakeString.data(), inTextLength);
		}
	return inDC.GetTextWidth(inText, inTextLength);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITextEngine_Z::Tracker

class ZUITextEngine_Z::Tracker : public ZMouseTracker
	{
public:
	Tracker(ZUITextEngine_Z* inTextEngine, ZPoint inStartPoint, bool inExtendSelection);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	ZUITextEngine_Z* fTextEngine;
	bool fExtendSelection;
	};

ZUITextEngine_Z::Tracker::Tracker(ZUITextEngine_Z* inTextEngine, ZPoint inStartPoint, bool inExtendSelection)
:	ZMouseTracker(inTextEngine->GetHost()->GetHostPane(inTextEngine), inStartPoint), fTextEngine(inTextEngine), fExtendSelection(inExtendSelection)
	{}

void ZUITextEngine_Z::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	size_t currentOffset = fTextEngine->PointToOffset(fTextEngine->FromHost(inNext));
	switch (inPhase)
		{
		case ePhaseBegin:
			{
			if (!fExtendSelection)
				fTextEngine->Internal_SetSelection(currentOffset, currentOffset);
			}
		case ePhaseContinue:
		case ePhaseEnd:
			{
			size_t selectPrimary, selectSecondary;
			fTextEngine->Internal_GetSelection(selectPrimary, selectSecondary);
			fTextEngine->Internal_SetSelection(selectPrimary, currentOffset);
			}
		}
	}

// =================================================================================================

#pragma mark -
#pragma mark * ZUITextEngine_Z
ZUITextEngine_Z::ZUITextEngine_Z(const ZDCFont& inFont, char inBulletChar)
	{
	this->Internal_Initialize();
	fFont = inFont;
	fBulletChar = inBulletChar;
	fPassword = true;
	}

ZUITextEngine_Z::ZUITextEngine_Z(const ZDCFont& inFont)
	{
	this->Internal_Initialize();
	fFont = inFont;
	fPassword = false;
	}

void ZUITextEngine_Z::Internal_Initialize()
	{
	fColor = ZRGBColor::sBlack;

	fSelectPrimary = 0;
	fSelectSecondary = 0;

	fLastCaretFlash = 0;
	fCaretShown = true;

	fDragOffset = 0;
	fDragActive = false;
	fDragBorderVisible = false;

	fChangeCounter = 0;
	fLastKnownChangeCounter = 0;
	fLastKnownSelectPrimary = fSelectPrimary;
	fLastKnownSelectSecondary = fSelectSecondary;
	}

ZUITextEngine_Z::~ZUITextEngine_Z()
	{
	}

void ZUITextEngine_Z::Draw(const ZDC& inDC)
	{
	if (fText.empty())
		return;

	ZDC localDC(inDC);
	localDC.PenNormal();
	localDC.SetFont(fFont);
//	localDC.SetTextColor(fColor);

	ZPoint mySize;
	ZCoord lineWidth;
	fHost->GetEngineSizes(this, mySize, lineWidth);

	bool isActive = fHost->GetEngineActive(this);
	bool isSelectionShown = fHost->GetEngineEditable(this);

	ZPoint lineOrigin = ZPoint::sZero;

	ZCoord ascent, descent, leading;
	localDC.GetFontInfo(ascent, descent, leading);
	lineOrigin.v += ascent;

	// set up the ink for hiliting the background
	localDC.SetInk(ZUIAttributeFactory::sGet()->GetInk_BackgroundHighlight()->GetInk());

	// Get the color for hilited text
	ZRGBColor textHilightColor = ZUIAttributeFactory::sGet()->GetColor_TextHighlight()->GetColor();

	size_t selectStart = min(fSelectPrimary, fSelectSecondary);
	size_t selectEnd = max(fSelectPrimary, fSelectSecondary);


	ZDCRgn hiliteRgn;
	const char* textStart = &fText.at(0);
	size_t currentLineOffset = 0;
	size_t remainingTextLength = fText.size();
	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
		sBreakLine(localDC, fPassword, fBulletChar, textStart + currentLineOffset, remainingTextLength, lineWidth, nextLineDelta);
		size_t nextLineOffset = currentLineOffset + nextLineDelta;

		// Draw from line start up to selectstart
		size_t drawnSelectStart = selectStart;
		if (drawnSelectStart < currentLineOffset)
			drawnSelectStart = currentLineOffset;
		if (drawnSelectStart >= nextLineOffset)
			drawnSelectStart = nextLineOffset;

		size_t drawnSelectEnd = selectEnd;
		if (drawnSelectEnd < currentLineOffset)
			drawnSelectEnd = currentLineOffset;
		if (drawnSelectEnd >= nextLineOffset)
			drawnSelectEnd = nextLineOffset;

		if (drawnSelectStart - currentLineOffset > 0)
			{
			localDC.SetTextColor(fColor);
			sDrawText(localDC, fPassword, fBulletChar, lineOrigin, textStart + currentLineOffset, drawnSelectStart - currentLineOffset);
			lineOrigin.h += sGetTextWidth(localDC, fPassword, fBulletChar, textStart + currentLineOffset, drawnSelectStart - currentLineOffset);
			}

		if (drawnSelectEnd - drawnSelectStart > 0)
			{
			ZCoord runWidth = sGetTextWidth(localDC, fPassword, fBulletChar, textStart + drawnSelectStart, drawnSelectEnd - drawnSelectStart);
			ZRect hiliteRect;
			if (drawnSelectStart == currentLineOffset)
				hiliteRect.left = 0;
			else
				hiliteRect.left = lineOrigin.h;

			if (drawnSelectEnd == nextLineOffset)
				hiliteRect.right = lineWidth;
			else
				hiliteRect.right = lineOrigin.h + runWidth;

			hiliteRect.top = lineOrigin.v - ascent;
			hiliteRect.bottom = lineOrigin.v + descent;

			if (isSelectionShown)
				{
				if (isActive)
					{
					localDC.Fill(hiliteRect);
					localDC.SetTextColor(textHilightColor);
					}
				else
					{
					localDC.SetTextColor(fColor);
					hiliteRgn |= hiliteRect;
					}
				}
			sDrawText(localDC, fPassword, fBulletChar, lineOrigin, textStart + drawnSelectStart, drawnSelectEnd - drawnSelectStart);
			lineOrigin.h += runWidth;
			}

		if (nextLineOffset - drawnSelectEnd > 0)
			{
			localDC.SetTextColor(fColor);
			sDrawText(localDC, fPassword, fBulletChar, lineOrigin, textStart + drawnSelectEnd, nextLineOffset - drawnSelectEnd);
			lineOrigin.h += sGetTextWidth(localDC, fPassword, fBulletChar, textStart + drawnSelectEnd, nextLineOffset - drawnSelectEnd);
			}

		lineOrigin.h = 0;
		lineOrigin.v += ascent + descent;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}

	if (!isActive)
		localDC.Frame(hiliteRgn);

	if (isActive && fCaretShown && fSelectPrimary == fSelectSecondary)
		this->Internal_ToggleDrawCaretAt(localDC, fSelectPrimary);

	if (fDragBorderVisible)
		this->Internal_ToggleDragBorder(localDC);

	if (fDragActive)
		this->Internal_ToggleDragCaret(localDC, fDragOffset);
	}

void ZUITextEngine_Z::DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength)
	{
	this->Draw(inDC);
	}

bool ZUITextEngine_Z::Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZUITextEngine::Click(inHitPoint, inEvent))
		return true;

	Tracker* theTracker = new Tracker(this, inHitPoint, inEvent.GetShift());
	theTracker->Install();

	return true;
	}

bool ZUITextEngine_Z::KeyDown(const ZEvent_Key& inEvent)
	{
	// Distinguish between editing keys and stuff to insert.
	size_t selectPrimary, selectSecondary;
	this->Internal_GetSelection(selectPrimary, selectSecondary);
	size_t selectStart = min(selectPrimary, selectSecondary);
	size_t selectEnd = max(selectPrimary, selectSecondary);
	size_t overallLength = this->GetTextLength();

	char theCharCode = inEvent.GetCharCode() & 0xFF;

	if (theCharCode == ZKeyboard::ccClear)
		theCharCode = ZKeyboard::ccFwdDelete;

	switch (theCharCode)
		{
		case ZKeyboard::ccLeft:
			{
			if (inEvent.GetShift())
				{
				if (selectSecondary > 0)
					this->Internal_SetSelection(selectPrimary, selectSecondary - 1);
				}
			else
				{
				if (selectPrimary == selectSecondary)
					{
					if (selectSecondary > 0)
						this->Internal_SetSelection(selectSecondary - 1, selectSecondary - 1);
					}
				else
					{
					this->Internal_SetSelection(selectStart, selectStart);
					}
				}
			}
			break;
		case ZKeyboard::ccRight:
			{
			if (inEvent.GetShift())
				{
				if (selectSecondary < fText.size())
					this->Internal_SetSelection(selectPrimary, selectSecondary + 1);
				}
			else
				{
				if (selectPrimary == selectSecondary)
					{
					if (selectSecondary < fText.size())
						this->Internal_SetSelection(selectSecondary + 1, selectSecondary + 1);
					}
				else
					{
					this->Internal_SetSelection(selectEnd, selectEnd);
					}
				}
			}
			break;
		case ZKeyboard::ccUp:
			{
			ZDC localDC(ZDCScratch::sGet());
			ZCoord ascent, descent, leading;
			localDC.GetFontInfo(ascent, descent, leading);

			ZPoint currentOffsetLocation = this->Internal_OffsetToPoint(fSelectSecondary);
			size_t newSecondaryOffset = this->PointToOffset(ZPoint(currentOffsetLocation.h, currentOffsetLocation.v - ascent - descent));
			if (inEvent.GetShift())
				{
				this->Internal_SetSelection(selectPrimary, newSecondaryOffset);
				}
			else
				{
				if (selectPrimary == selectSecondary)
					this->Internal_SetSelection(newSecondaryOffset, newSecondaryOffset);
				else
					this->Internal_SetSelection(selectStart, selectStart);
				}
			}
			break;
		case ZKeyboard::ccDown:
			{
			ZDC localDC(ZDCScratch::sGet());
			ZCoord ascent, descent, leading;
			localDC.GetFontInfo(ascent, descent, leading);

			ZPoint currentOffsetLocation = this->Internal_OffsetToPoint(fSelectSecondary);
			size_t newSecondaryOffset = this->PointToOffset(ZPoint(currentOffsetLocation.h, currentOffsetLocation.v + ascent + descent));
			if (inEvent.GetShift())
				{
				this->Internal_SetSelection(selectPrimary, newSecondaryOffset);
				}
			else
				{
				if (selectPrimary == selectSecondary)
					this->Internal_SetSelection(newSecondaryOffset, newSecondaryOffset);
				else
					this->Internal_SetSelection(selectEnd, selectEnd);
				}
			}
			break;
		case ZKeyboard::ccPageUp:
			break;
		case ZKeyboard::ccPageDown:
			break;
		case ZKeyboard::ccHome:
			this->Internal_SetSelection(0, 0);
			break;
		case ZKeyboard::ccEnd:
			this->Internal_SetSelection(overallLength, overallLength);
			break;
		case ZKeyboard::ccFwdDelete:
			{
			if (selectPrimary == selectSecondary)
				{
				if (selectPrimary < fText.size())
					{
					if (this->IsRangeEditable(selectPrimary, 1))
						this->DeleteText(selectPrimary, 1);
					}
				}
			else
				{
				if (this->IsRangeEditable(selectStart, selectEnd - selectStart))
					{
					this->DeleteText(selectStart, selectEnd - selectStart);
					//this->Internal_SetSelection(selectStart, selectStart);
					}
				}
			}
			break;
		case ZKeyboard::ccBackspace:
			{
			if (selectPrimary == selectSecondary)
				{
				if (selectPrimary > 0)
					{
					if (this->IsRangeEditable(selectPrimary - 1, 1))
						{
						this->DeleteText(selectPrimary - 1, 1);
						this->Internal_SetSelection(selectPrimary - 1, selectPrimary - 1);
						}
					}
				}
			else
				{
				if (this->IsRangeEditable(selectStart, selectEnd - selectStart))
					{
					this->DeleteText(selectStart, selectEnd - selectStart);
					this->Internal_SetSelection(selectStart, selectStart);
					}
				}
			}
			break;
		default:
			{
			if (this->IsRangeEditable(selectStart, selectEnd - selectStart))
				this->ReplaceSelection(&theCharCode, 1);
			}
			break;
		}
	return true;
	}

void ZUITextEngine_Z::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	ZUITextEngine::MouseMotion(inPoint, inMotion);
	}

void ZUITextEngine_Z::Activate(bool entering)
	{
	this->Internal_Invalidate();
	}

void ZUITextEngine_Z::Idle(const ZDC& inDC, bool inEnabledAndActive)
	{
	if (fLastKnownChangeCounter != fChangeCounter)
		{
		fLastKnownChangeCounter = fChangeCounter;
		static_cast<ZUISelection*>(this)->Internal_NotifyResponders(ZUISelection::ContentChanged);
		}

	size_t selectStart, selectLength;
	this->GetSelection(selectStart, selectLength);

	if (fLastKnownSelectPrimary != fSelectPrimary || fLastKnownSelectSecondary != fSelectSecondary)
		{
		fLastKnownSelectPrimary = fSelectPrimary;
		fLastKnownSelectSecondary = fSelectSecondary;
		static_cast<ZUISelection*>(this)->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}

	if (inEnabledAndActive && fSelectPrimary == fSelectSecondary)
		{
		bigtime_t now = ZTicks::sNow();
		if (now - fLastCaretFlash > ZTicks::sCaretFlash())
			{
			ZDC localDC(inDC);
			localDC.SetFont(fFont);

			this->Internal_ToggleDrawCaretAt(localDC, fSelectPrimary);
			fLastCaretFlash = now;
			fCaretShown = !fCaretShown;
			}
		}
	}

void ZUITextEngine_Z::SetDragOffset(size_t inDragOffset, bool inDragActive)
	{
	if (fDragOffset == inDragOffset && fDragActive == inDragActive)
		return;

	// We're changing stuff, so hide the old caret
	ZDC theDC = this->GetHostDC();

	if (fDragActive)
		this->Internal_ToggleDragCaret(theDC, fDragOffset);
	fDragActive = inDragActive;
	fDragOffset = inDragOffset;
	if (fDragActive)
		this->Internal_ToggleDragCaret(theDC, fDragOffset);
	}

void ZUITextEngine_Z::SetDragBorderVisible(bool inVisible)
	{
	if (fDragBorderVisible != inVisible)
		{
		fDragBorderVisible = inVisible;
		this->Internal_ToggleDragBorder(this->GetHostDC());
		}
	}

void ZUITextEngine_Z::SetFont(const ZDCFont& inFont)
	{
	fFont = inFont;
	++fChangeCounter;
	this->Internal_Invalidate();
	}

void ZUITextEngine_Z::SetTextColor(ZRGBColor inTextColor)
	{
	fColor = inTextColor;
	++fChangeCounter;
	this->Internal_Invalidate();
	}

ZRGBColor ZUITextEngine_Z::GetTextColor()
	{
	return fColor;
	}

void ZUITextEngine_Z::SetBackColor(ZRGBColor inBackColor)
	{
	}

ZRGBColor ZUITextEngine_Z::GetBackColor()
	{
	return ZRGBColor::sWhite;
	}

size_t ZUITextEngine_Z::PointToOffset(ZPoint inPoint)
	{
	if (fText.empty() || inPoint.v < 0)
		return 0;

	ZDC localDC(ZDCScratch::sGet());
	localDC.SetFont(fFont);

	ZPoint mySize;
	ZCoord lineWidth;
	fHost->GetEngineSizes(this, mySize, lineWidth);

	ZCoord ascent, descent, leading;
	localDC.GetFontInfo(ascent, descent, leading);

	ZCoord currentLineTop = 0;

	const char* textStart = &fText.at(0);
	size_t currentLineOffset = 0;
	size_t remainingTextLength = fText.size();
	while (remainingTextLength > 0)
		{
		ZCoord currentLineBottom = currentLineTop + ascent + descent;

		size_t nextLineDelta;
		sBreakLine(localDC, fPassword, fBulletChar, textStart + currentLineOffset, remainingTextLength, lineWidth, nextLineDelta);
		size_t nextLineOffset = currentLineOffset + nextLineDelta;

		if (inPoint.v >= currentLineTop && inPoint.v < currentLineBottom)
			{
			// We've moved down vertically enough, figure out how far into the line inPoint.h is
			ZCoord currentGlyphLeftEdge = 0;
			size_t currentGlyphOffset = currentLineOffset;
			while (true)
				{
				if (currentGlyphOffset >= nextLineOffset || currentGlyphLeftEdge >= inPoint.h)
					return currentGlyphOffset;
				currentGlyphLeftEdge += sGetTextWidth(localDC, fPassword, fBulletChar, textStart + currentGlyphOffset, 1);
				currentGlyphOffset += 1;
				}
			}
		currentLineTop = currentLineBottom;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}
	return fText.size();
	}

ZDCRgn ZUITextEngine_Z::GetTextRgn(size_t inOffset, size_t inLength)
	{
	ZDCRgn textRgn;
	if (fText.empty())
		return textRgn;

	ZDC localDC(ZDCScratch::sGet());
	localDC.SetFont(fFont);

	ZPoint mySize;
	ZCoord lineWidth;
	fHost->GetEngineSizes(this, mySize, lineWidth);

	ZPoint lineOrigin = ZPoint::sZero;

	ZCoord ascent, descent, leading;
	localDC.GetFontInfo(ascent, descent, leading);
	lineOrigin.v += ascent;

	// set up the ink for hiliting the background
	size_t selectStart = inOffset;
	size_t selectEnd = inOffset + inLength;

	const char* textStart = fText.data();
	size_t currentLineOffset = 0;
	size_t remainingTextLength = fText.size();
	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
		sBreakLine(localDC, fPassword, fBulletChar, textStart + currentLineOffset, remainingTextLength, lineWidth, nextLineDelta);
		size_t nextLineOffset = currentLineOffset + nextLineDelta;
		// Draw from line start up to selectstart
		size_t drawnSelectStart = selectStart;
		if (drawnSelectStart < currentLineOffset)
			drawnSelectStart = currentLineOffset;
		if (drawnSelectStart >= nextLineOffset)
			drawnSelectStart = nextLineOffset;

		size_t drawnSelectEnd = selectEnd;
		if (drawnSelectEnd < currentLineOffset)
			drawnSelectEnd = currentLineOffset;
		if (drawnSelectEnd >= nextLineOffset)
			drawnSelectEnd = nextLineOffset;

		if (drawnSelectStart - currentLineOffset > 0)
			lineOrigin.h += sGetTextWidth(localDC, fPassword, fBulletChar, textStart + currentLineOffset, drawnSelectStart - currentLineOffset);
		if (drawnSelectEnd - drawnSelectStart > 0)
			{
			ZCoord runWidth = sGetTextWidth(localDC, fPassword, fBulletChar, textStart + drawnSelectStart, drawnSelectEnd - drawnSelectStart);
			ZRect hiliteRect;
			if (drawnSelectStart == currentLineOffset)
				hiliteRect.left = 0;
			else
				hiliteRect.left = lineOrigin.h;
			if (drawnSelectEnd == nextLineOffset)
				hiliteRect.right = lineWidth;
			else
				hiliteRect.right = lineOrigin.h + runWidth;
			hiliteRect.top = lineOrigin.v - ascent;
			hiliteRect.bottom = lineOrigin.v + descent;
			textRgn |= hiliteRect;
			lineOrigin.h += runWidth;
			}
		if (nextLineOffset - drawnSelectEnd > 0)
			lineOrigin.h += sGetTextWidth(localDC, fPassword, fBulletChar, textStart + drawnSelectEnd, nextLineOffset - drawnSelectEnd);

		lineOrigin.h = 0;
		lineOrigin.v += ascent + descent;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}
	return textRgn;
	}

void ZUITextEngine_Z::HandleDrag(ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag)
	{
	ZUITextEngine::HandleDrag(inPoint, inMotion, inDrag);
	}

void ZUITextEngine_Z::HandleDrop(ZPoint inPoint, ZDrop& inDrop)
	{
	ZUITextEngine::HandleDrop(inPoint, inDrop);
	}

bool ZUITextEngine_Z::AcceptsDragDrop(const ZDragDropBase& inDragDrop)
	{
	return ZUITextEngine::AcceptsDragDrop(inDragDrop);
	}

ZTuple ZUITextEngine_Z::GetTuple(size_t inOffset, size_t inLength)
	{
	return ZUITextEngine::GetTuple(inOffset, inLength);
	}

long ZUITextEngine_Z::GetUserChangeCount()
	{
	return fChangeCounter;
	}

bool ZUITextEngine_Z::IsRangeEditable(size_t inOffset, size_t inLength)
	{
	return ZUITextEngine::IsRangeEditable(inOffset, inLength);
	}

size_t ZUITextEngine_Z::GetTextLength()
	{
	return fText.size();
	}

void ZUITextEngine_Z::GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength)
	{
	ZAssert(inOffset + inLength <= fText.size());
	if (inLength > 0)
		ZBlockCopy(fText.data() + inOffset, outDest, inLength);

	outLength = inLength;
	}

void ZUITextEngine_Z::SetText_Impl(const char* inSource, size_t inLength)
	{
	fText = string(inSource, inLength);
	this->Internal_Invalidate();
	++fChangeCounter;
	}

void ZUITextEngine_Z::InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength)
	{
	fText = fText.substr(0, inInsertOffset)
			+ string(inSource, inLength)
			+ fText.substr(inInsertOffset, fText.size() - inInsertOffset);

	this->Internal_Invalidate();
	++fChangeCounter;
	}

void ZUITextEngine_Z::DeleteText(size_t inOffset, size_t inLength)
	{
	ZAssert(inOffset + inLength <= fText.size());
	if (inLength == 0)
		return;

	size_t oldStart = min(fSelectPrimary, fSelectSecondary);
	size_t oldEnd = max(fSelectPrimary, fSelectSecondary);

	size_t deletedStart = inOffset;
	size_t deletedEnd = inOffset+inLength;

	if (deletedStart >= oldEnd)
		{
		// Entirely after the selection, do nothing
		}
	else if (deletedEnd <= oldStart)
		{
		// Entirely before the selection
		fSelectPrimary -= inLength;
		fSelectSecondary -= inLength;
		}
	else if (deletedStart <= oldStart && deletedEnd >= oldEnd)
		{
		// Completely encompassing the selection
		fSelectPrimary = deletedStart;
		fSelectSecondary = fSelectPrimary;
		}
	else if (deletedStart >= oldStart && deletedEnd <= oldEnd)
		{
		// Entirely within the selection
		if (fSelectSecondary > fSelectPrimary)
			fSelectSecondary -= inLength;
		else
			fSelectPrimary -= inLength;
		}
	else if (deletedStart <= oldStart && deletedEnd <= oldEnd)
		{
		// Overlap at the beginning (or exactly matches)
		if (fSelectPrimary < fSelectSecondary)
			{
			fSelectPrimary = deletedStart;
			fSelectSecondary -= inLength;
			}
		else
			{
			fSelectSecondary = deletedStart;
			fSelectPrimary -= inLength;
			}
		}
	else if (deletedStart >= oldStart && deletedEnd >= oldEnd)
		{
		// Overlap at the end (or exactly matches, although that'll be handled by the previous case)
		if (fSelectPrimary < fSelectSecondary)
			fSelectSecondary = deletedStart;
		else
			fSelectPrimary = deletedStart;
		}
	else
		{
		// Hmmm -- got some other case -- logic must be flawed
		ZUnimplemented();
		}

	fText = fText.substr(0, deletedStart) + fText.substr(deletedEnd, fText.size() - deletedEnd);

	this->Internal_Invalidate();
	++fChangeCounter;
	}

void ZUITextEngine_Z::ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength)
	{
	fText = fText.substr(0, inReplaceOffset) + string(inSource, inLength) + fText.substr(inReplaceOffset + inReplaceLength, fText.size() - (inReplaceOffset + inReplaceLength));
	this->Internal_Invalidate();
	++fChangeCounter;
	}

void ZUITextEngine_Z::GetSelection(size_t& outOffset, size_t& outLength)
	{
	outOffset = min(fSelectPrimary, fSelectSecondary);
	outLength = max(fSelectPrimary, fSelectSecondary) - outOffset;
	}

void ZUITextEngine_Z::SetSelection(size_t inOffset, size_t inLength)
	{
	this->Internal_SetSelection(inOffset, inOffset + inLength);
	}

void ZUITextEngine_Z::ReplaceSelection_Impl(const char* inSource, size_t inLength)
	{
	size_t selectStart = min(fSelectPrimary, fSelectSecondary);
	size_t selectEnd = max(fSelectPrimary, fSelectSecondary);
	fText = fText.substr(0, selectStart) + string(inSource, inLength) + fText.substr(selectEnd, fText.size() - selectEnd);
	fSelectPrimary = selectStart + inLength;
	fSelectSecondary = fSelectPrimary;
	this->Internal_Invalidate();
	++fChangeCounter;
	}

void ZUITextEngine_Z::ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications)
	{
	}

void ZUITextEngine_Z::ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications)
	{
	}

void ZUITextEngine_Z::ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	}

void ZUITextEngine_Z::ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	}

ZPoint_T<double> ZUITextEngine_Z::GetScrollValues()
	{
	return ZPoint_T<double>(0,0);
	}

ZPoint_T<double> ZUITextEngine_Z::GetScrollThumbProportions()
	{
	return ZPoint_T<double>(1,1);
	}

ZDCFont ZUITextEngine_Z::GetFont()
	{
	return fFont;
	}

void ZUITextEngine_Z::Internal_Invalidate()
	{
	ZPoint mySize;
	ZCoord lineWidth;
	fHost->GetEngineSizes(this, mySize, lineWidth);
	fHost->GetHostPane(this)->Invalidate(ZRect(mySize) + fHost->GetEngineLocation(this));
	}

void ZUITextEngine_Z::Internal_GetSelection(size_t& outSelectPrimary, size_t& outSelectSecondary)
	{
	outSelectPrimary = fSelectPrimary;
	outSelectSecondary = fSelectSecondary;
	}

void ZUITextEngine_Z::Internal_SetSelection(size_t inSelectPrimary, size_t inSelectSecondary)
	{
	size_t realPrimary = min(fText.size(), inSelectPrimary);
	size_t realSecondary = min(fText.size(), inSelectSecondary);
	if (fSelectPrimary == realPrimary && fSelectSecondary == realSecondary)
		return;
	fSelectPrimary = realPrimary;
	fSelectSecondary = realSecondary;
	this->Internal_Invalidate();
	}

void ZUITextEngine_Z::Internal_ToggleDrawCaretAt(const ZDC& inDC, size_t inCaretOffset)
	{
	ZCoord ascent, descent, leading;
	inDC.GetFontInfo(ascent, descent, leading);

	ZPoint thePoint = this->Internal_OffsetToPoint(inCaretOffset);
	inDC.Invert(ZRect(thePoint.h, thePoint.v, thePoint.h + 1, thePoint.v + ascent + descent));
	}

ZPoint ZUITextEngine_Z::Internal_OffsetToPoint(size_t inOffset)
	{
	if (fText.empty())
		return ZPoint::sZero;

	ZDC localDC(ZDCScratch::sGet());
	localDC.SetFont(fFont);

	ZPoint mySize;
	ZCoord lineWidth;
	fHost->GetEngineSizes(this, mySize, lineWidth);

	ZCoord ascent, descent, leading;
	localDC.GetFontInfo(ascent, descent, leading);

	ZCoord currentLineTop = 0;

	const char* textStart = &fText.at(0);
	size_t currentLineOffset = 0;
	size_t remainingTextLength = fText.size();
	while (remainingTextLength > 0)
		{
		ZCoord currentLineBottom = currentLineTop + ascent + descent;

		size_t nextLineDelta;
		sBreakLine(localDC, fPassword, fBulletChar, textStart + currentLineOffset, remainingTextLength, lineWidth, nextLineDelta);
		size_t nextLineOffset = currentLineOffset + nextLineDelta;
		if (inOffset >= currentLineOffset && inOffset <= nextLineOffset)
			{
			// We've moved down vertically enough, figure out how far into the line inOffset.h is
			ZCoord theWidth = sGetTextWidth(localDC, fPassword, fBulletChar, textStart + currentLineOffset, inOffset - currentLineOffset);
			return ZPoint(theWidth, currentLineTop);
			}
		currentLineTop = currentLineBottom;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}
	return ZPoint(0, currentLineTop);
	}

void ZUITextEngine_Z::Internal_ToggleDragCaret(const ZDC& inDC, size_t inDragOffset)
	{
	this->Internal_ToggleDrawCaretAt(inDC, inDragOffset);
	}

void ZUITextEngine_Z::Internal_ToggleDragBorder(const ZDC& inDC)
	{
	ZDCRgn borderRgn = this->GetHost()->GetHostPane(this)->CalcApertureRgn();
	ZUtil_Graphics::sDrawDragDropHilite(inDC, borderRgn);
	}

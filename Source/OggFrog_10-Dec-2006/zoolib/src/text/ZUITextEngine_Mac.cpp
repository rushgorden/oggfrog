static const char rcsid[] = "@(#) $Id: ZUITextEngine_Mac.cpp,v 1.5 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUITextEngine_Mac.h"

// =================================================================================================
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

#include "ZDC_QD.h"
#include "ZMemory.h"
#include "ZUtil_Graphics.h"

class ZUITextEngine_TextEdit::BeInPort
	{
public:
	BeInPort(ZDC& inDC, ZUITextEngine_TextEdit* inTextEngine);
	BeInPort(ZUITextEngine_TextEdit* inTextEngine);
	~BeInPort();
private:
	ZUITextEngine_TextEdit* fTextEngine;
	ZDCSetupForQD fSetupForQD;
	GrafPtr fOldPort;
	};

// ==================================================

static ZDC& sStuffDC(ZDC& inDC)
	{
	inDC.SetInk(ZDCInk::sWhite);
	return inDC;
	}

ZUITextEngine_TextEdit::BeInPort::BeInPort(ZDC& inDC, ZUITextEngine_TextEdit* inTextEngine)
:	fSetupForQD(inDC, false),
	fTextEngine(inTextEngine)
	{
	ZAssert(fTextEngine);
	fOldPort = fTextEngine->fTEHandle[0]->inPort;
	RGBColor theForeColor(fTextEngine->GetTextColor());
	RGBColor theBackColor(fTextEngine->GetBackColor());
	::PenNormal();
	::RGBForeColor(&theForeColor);
	::RGBBackColor(&theBackColor);
	fTextEngine->fTEHandle[0]->inPort = fSetupForQD.GetGrafPtr();
	}

ZUITextEngine_TextEdit::BeInPort::BeInPort(ZUITextEngine_TextEdit* inTextEngine)
:	fSetupForQD(inTextEngine->GetHostDC(), false),
	fTextEngine(inTextEngine)
	{
	ZAssert(fTextEngine);

	fOldPort = fTextEngine->fTEHandle[0]->inPort;
	RGBColor theForeColor(fTextEngine->GetTextColor());
	RGBColor theBackColor(fTextEngine->GetBackColor());
	::PenNormal();
	::RGBForeColor(&theForeColor);
	::RGBBackColor(&theBackColor);
	fTextEngine->fTEHandle[0]->inPort = fSetupForQD.GetGrafPtr();
	}

ZUITextEngine_TextEdit::BeInPort::~BeInPort()
	{
	fTextEngine->fTEHandle[0]->inPort = fOldPort;
	}

// =================================================================================================
// =================================================================================================

#if ZCONFIG(OS, MacOS7)
TEClickLoopUPP ZUITextEngine_TextEdit::sTEClickLoopUPP = NewTEClickLoopUPP(ZUITextEngine_TextEdit::sTEClickLoop);
ZUITextEngine_TextEdit* ZUITextEngine_TextEdit::sCurrentTextEngine = nil;
#endif // ZCONFIG(OS, MacOS7)

ZUITextEngine_TextEdit::ZUITextEngine_TextEdit(const ZDCFont& theFont)
:	fTextColor(ZRGBColor::sBlack), fBackColor(ZRGBColor::sWhite), fDragOffset(0), fDragActive(false), fDragBorderVisible(false)
	{
	fChangeCounter = 0;
	fLastKnownChangeCounter = 0;
	fLastKnownSelectionOffset = 0;
	fLastKnownSelectionLength = 0;

	ZDC theDCScratch = ZDCScratch::sGet();
	ZDCSetupForQD theSetupForQD(theDCScratch, true);
	::TextFont(theFont.GetFontID());
	::TextSize(theFont.GetSize());
	::TextFace(theFont.GetStyle());
	Rect dummy = ZRect(20,20);
	fTEHandle = ::TENew(&dummy, &dummy);
	if (fTEHandle == nil)
		throw bad_alloc();
	::TEFeatureFlag(teFOutlineHilite, teBitSet, fTEHandle);
	}
ZUITextEngine_TextEdit::~ZUITextEngine_TextEdit()
	{
	if (fTEHandle)
		::TEDispose(fTEHandle); fTEHandle = nil;
	}

void ZUITextEngine_TextEdit::Draw(const ZDC& inDC)
	{
	this->Internal_CheckGeometry();

	ZDC localDC(inDC);
	localDC.SetInk(fBackColor);
	localDC.Fill(localDC.GetClip().Bounds());
	BeInPort theBeInPort(localDC, this);
	Rect bounds = fTEHandle[0]->viewRect;
	::TEUpdate(&bounds, fTEHandle);
	if (fDragBorderVisible)
		this->Internal_ToggleDragBorder(localDC);
	if (fDragActive)
		this->Internal_ToggleDragCaret(localDC, fDragOffset);
	}
void ZUITextEngine_TextEdit::DrawRange(const ZDC& inDC, size_t inOffset, size_t inLength)
	{
	this->Draw(inDC);
	}
bool ZUITextEngine_TextEdit::Click(ZPoint theHitPoint, const ZEvent_Mouse& theEvent)
	{
	this->Internal_CheckGeometry();

	if (ZUITextEngine::Click(theHitPoint, theEvent))
		return true;

//	CheckScroll theCheckScroll(this);

	BeInPort theBeInPort(this);

//	ZAssert(sCurrentTextEngine == nil);
//	sCurrentTextEngine = this;
	bool oldAutoScroll = ::TEFeatureFlag(teFAutoScroll, teBitSet, fTEHandle);
//	::TESetClickLoop(sTEClickLoopUPP, fTEHandle);
	::TEClick(this->FromHost(theHitPoint), theEvent.GetShift(), fTEHandle);
	if (!oldAutoScroll)
		::TEFeatureFlag(teFAutoScroll, teBitClear, fTEHandle);
//	sCurrentTextEngine = nil;

	return true;
	}
bool ZUITextEngine_TextEdit::KeyDown(const ZEvent_Key& theEvent)
	{
	if (theEvent.GetCommand())
		return false;

	size_t currentSelectOffset, currentSelectLength;
	this->GetSelection(currentSelectOffset, currentSelectLength);
	size_t overallLength = this->GetTextLength();

	unsigned char theCharCode = theEvent.GetCharCode() & 0xFF;

	if (theCharCode == ZKeyboard::ccClear)
		theCharCode = ZKeyboard::ccFwdDelete;

	bool allowKeyStroke = true;
	switch (theCharCode)
		{
		case ZKeyboard::ccRight:
			break;
		case ZKeyboard::ccLeft:
			break;
		case ZKeyboard::ccUp:
			break;
		case ZKeyboard::ccDown:
			break;
		case ZKeyboard::ccPageUp:
			return true;
		case ZKeyboard::ccPageDown:
			return true;
		case ZKeyboard::ccHome:
			return true;
		case ZKeyboard::ccEnd:
			return true;
		case ZKeyboard::ccBackspace:
			{
			if (currentSelectLength == 0)
				{
				if (currentSelectOffset == 0)
					allowKeyStroke = false;
				else if (!this->IsRangeEditable(currentSelectOffset - 1, 1))
					allowKeyStroke = false;
				}
			else
				{
				if (!this->IsRangeEditable(currentSelectOffset, currentSelectLength))
					allowKeyStroke = false;
				}
			}
			break;
		case ZKeyboard::ccFwdDelete:
			{
			if (currentSelectLength == 0)
				{
				if (currentSelectOffset >= overallLength)
					allowKeyStroke = false;
				else if (!this->IsRangeEditable(currentSelectOffset, 1))
					allowKeyStroke = false;
				}
			else
				{
				if (!this->IsRangeEditable(currentSelectOffset, currentSelectLength))
					allowKeyStroke = false;
				}
			}
			break;
		default:
			{
			if (!this->IsRangeEditable(currentSelectOffset, currentSelectLength))
				allowKeyStroke = false;
			}
			break;
		}
	if (!allowKeyStroke)
		return true;

	this->Internal_CheckGeometry();

	CheckScroll theCheckScroll(this);

	BeInPort theBeInPort(this);
	bool oldAutoScroll = ::TEFeatureFlag(teFAutoScroll, teBitSet, fTEHandle);
	::TEKey(theEvent.GetCharCode(), fTEHandle);
	if (!oldAutoScroll)
		::TEFeatureFlag(teFAutoScroll, teBitClear, fTEHandle);
	++fChangeCounter;
	return true;
	}
void ZUITextEngine_TextEdit::Idle(const ZDC& inDC, bool inEnabledAndActive)
	{
	if (fLastKnownChangeCounter != fChangeCounter)
		{
		fLastKnownChangeCounter = fChangeCounter;
		static_cast<ZUISelection*>(this)->Internal_NotifyResponders(ZUISelection::ContentChanged);
		}
	size_t selectStart, selectLength;
	this->GetSelection(selectStart, selectLength);
	if (fLastKnownSelectionOffset != selectStart || fLastKnownSelectionLength != selectLength)
		{
		fLastKnownSelectionOffset = selectStart;
		fLastKnownSelectionLength = selectLength;
		static_cast<ZUISelection*>(this)->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}

	if (!inEnabledAndActive)
		return;
	if (fTEHandle[0]->selStart == fTEHandle[0]->selEnd)
		{
		this->Internal_CheckGeometry();
		ZDC localDC(inDC);
		BeInPort theBeInPort(localDC, this);
		::TEIdle(fTEHandle);
		}
	}

void ZUITextEngine_TextEdit::SetDragOffset(size_t inDragOffset, bool inDragActive)
	{
	if (fDragOffset == inDragOffset && fDragActive == inDragActive)
		return;
	this->Internal_CheckGeometry();

// We're changing stuff, so hide the old caret
	ZDC theDC = this->GetHostDC();

	if (fDragActive)
		this->Internal_ToggleDragCaret(theDC, fDragOffset);
	fDragActive = inDragActive;
	fDragOffset = inDragOffset;
	if (fDragActive)
		this->Internal_ToggleDragCaret(theDC, fDragOffset);
	}

void ZUITextEngine_TextEdit::SetDragBorderVisible(bool inVisible)
	{
	if (fDragBorderVisible != inVisible)
		{
		fDragBorderVisible = inVisible;
		this->Internal_ToggleDragBorder(this->GetHostDC());
		}
	}

void ZUITextEngine_TextEdit::Activate(bool entering)
	{
	this->Internal_CheckGeometry();

	BeInPort theBeInPort(this);
	if (entering)
		::TEActivate(fTEHandle);
	else
		::TEDeactivate(fTEHandle);
	}

size_t ZUITextEngine_TextEdit::GetTextLength()
	{
	return fTEHandle[0]->teLength;
	}

void ZUITextEngine_TextEdit::GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength)
	{
	ZAssert(inOffset + inLength <= ::GetHandleSize(fTEHandle[0]->hText));

	ZBlockCopy(fTEHandle[0]->hText[0] + inOffset, outDest, inLength);
	outLength = inLength;
	}

void ZUITextEngine_TextEdit::SetText_Impl(const char* inSource, size_t inLength)
	{
	CheckScroll theCheckScroll(this);

	this->Internal_CheckGeometry();

	BeInPort theBeInPort(this);
	::TESetSelect(0, fTEHandle[0]->teLength, fTEHandle);
	::TEDelete(fTEHandle);
	::TEInsert(inSource, inLength, fTEHandle);
	++fChangeCounter;
	}

void ZUITextEngine_TextEdit::InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength)
	{
	ZAssert(inInsertOffset <= fTEHandle[0]->teLength);

	short oldStart = fTEHandle[0]->selStart;
	short oldEnd = fTEHandle[0]->selEnd;

	BeInPort theBeInPort(this);
	::TESetSelect(inInsertOffset, inInsertOffset, fTEHandle);
	::TEInsert(inSource, inLength, fTEHandle);
	if (inInsertOffset > oldStart && inInsertOffset < oldEnd)
		::TESetSelect(oldStart, oldEnd + inLength, fTEHandle);
	else if (inInsertOffset <= oldStart)
		::TESetSelect(oldStart + inLength, oldEnd + inLength, fTEHandle);
	else
		::TESetSelect(oldStart, oldEnd, fTEHandle);
	++fChangeCounter;
	}

void ZUITextEngine_TextEdit::ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength)
	{
	this->Internal_CheckGeometry();

	CheckScroll theCheckScroll(this);

	this->DeleteText(inReplaceOffset, inReplaceLength);
	this->InsertText(inReplaceOffset, inSource, inLength);
	}

void ZUITextEngine_TextEdit::DeleteText(size_t inOffset, size_t inLength)
	{
	if (inLength == 0)
		return;

	CheckScroll theCheckScroll(this);

	this->Internal_CheckGeometry();

	ZAssert(inOffset <= fTEHandle[0]->teLength);
	ZAssert(inOffset + inLength <= fTEHandle[0]->teLength);

	short oldStart = fTEHandle[0]->selStart;
	short oldEnd = fTEHandle[0]->selEnd;

	short deletedStart = inOffset;
	short deletedEnd = inOffset + inLength;

	BeInPort theBeInPort(this);
	::TESetSelect(inOffset, inOffset + inLength, fTEHandle);
	::TEDelete(fTEHandle);
// Entirely after the selection
	if (deletedStart > oldEnd)
		::TESetSelect(oldStart, oldEnd, fTEHandle);
// Entirely before the selection
	else if (deletedEnd <= oldStart)
		::TESetSelect(oldStart - inLength, oldEnd - inLength, fTEHandle);
// Completely encompassing the selection
	else if (deletedStart <= oldStart && deletedEnd >= oldEnd)
		::TESetSelect(deletedStart, deletedStart, fTEHandle);
// Entirely within the selection
	else if (deletedStart >= oldStart && deletedEnd <= oldEnd)
		::TESetSelect(oldStart, oldEnd - inLength, fTEHandle);
// Overlap at the beginning
	else if (deletedStart < oldStart && deletedEnd < oldEnd)
		::TESetSelect(deletedStart, deletedStart + (oldEnd - oldStart) + (deletedEnd - oldStart), fTEHandle);
// Overlap at the end
	else if (deletedStart > oldStart && deletedEnd > oldEnd)
		::TESetSelect(oldStart, deletedStart, fTEHandle);
	else
		{
// Hmmm -- got some other case -- logic must be flawed
		ZUnimplemented();
		}
	++fChangeCounter;
	}

void ZUITextEngine_TextEdit::GetSelection(size_t& outOffset, size_t& outLength)
	{
	outOffset = fTEHandle[0]->selStart;
	outLength = fTEHandle[0]->selEnd - outOffset;
	}
void ZUITextEngine_TextEdit::SetSelection(size_t inOffset, size_t inLength)
	{
	ZAssert(inOffset <= fTEHandle[0]->teLength);
	ZAssert(inOffset + inLength <= fTEHandle[0]->teLength);

	this->Internal_CheckGeometry();

	BeInPort theBeInPort(this);
	::TESetSelect(inOffset, inOffset + inLength, fTEHandle);
	}

void ZUITextEngine_TextEdit::ReplaceSelection_Impl(const char* inSource, size_t inLength)
	{
	this->Internal_CheckGeometry();

	BeInPort theBeInPort(this);

	CheckScroll theCheckScroll(this);

	::TEDelete(fTEHandle);
	if (inLength)
		{
		ZAssert(inSource);
		::TEInsert(inSource, inLength, fTEHandle);
		}

	++fChangeCounter;
	}

#if ZCONFIG(OS, MacOS7)

bool ZUITextEngine_TextEdit::TEClickLoop()
	{
//	bool oldAutoScroll = ::TEFeatureFlag(teFAutoScroll, teBitSet, fTEHandle);
	::TESelView(fTEHandle);
//	if (!oldAutoScroll)
//		::TEFeatureFlag(teFAutoScroll, teBitClear, fTEHandle);
	return true;
	}

pascal Boolean ZUITextEngine_TextEdit::sTEClickLoop(TEPtr inTEPtr)
	{
	ZAssert(sCurrentTextEngine);

// Save the clip, origin etc.

	CGrafPtr oldPort;
	GDHandle oldGDHandle;
	::GetGWorld(&oldPort, &oldGDHandle);

	RgnHandle oldClip = ::NewRgn();
	::GetClip(oldClip);

	ZPoint oldOrigin;
	oldOrigin.h = oldPort->portRect.left;
	oldOrigin.v = oldPort->portRect.top;

	PenState oldPenState;
	::GetPenState(&oldPenState);
	RGBColor oldForeColor, oldBackColor;
	::GetForeColor(&oldForeColor);
	::GetBackColor(&oldBackColor);

	short oldTextMode = oldPort->txMode;
	short oldTextSize = oldPort->txSize;
	short oldTextFont = oldPort->txFont;
	short oldTextFace = oldPort->txFace;

	bool result = sCurrentTextEngine->TEClickLoop();

	::SetGWorld(oldPort, oldGDHandle);
	::RGBForeColor(&oldForeColor);
	::RGBBackColor(&oldBackColor);
	::SetPenState(&oldPenState);
	::TextMode(oldTextMode);
	::TextSize(oldTextSize);
	::TextFont(oldTextFont);
	::TextFace(oldTextFace);
	::SetOrigin(oldOrigin.h, oldOrigin.v);
	::SetClip(oldClip);
	::DisposeRgn(oldClip);

	return result;
	}

#endif // ZCONFIG(OS, MacOS7)


void ZUITextEngine_TextEdit::Internal_ScrollTo(ZPoint translation, bool inDrawIt, bool inDoNotifications)
	{
	CheckScroll theCheckScroll(this);

	ZRect destRect = fTEHandle[0]->destRect;
	ZPoint currentTranslation(-destRect.TopLeft());
	if (inDrawIt)
		{
		BeInPort theBeInPort(this);
		::TEPinScroll(currentTranslation.h - translation.h, currentTranslation.v - translation.v, fTEHandle);
		}
	else
		::TEPinScroll(currentTranslation.h - translation.h, currentTranslation.v - translation.v, fTEHandle);
	}

void ZUITextEngine_TextEdit::ScrollBy(ZPoint_T<double> delta, bool inDrawIt, bool inDoNotifications)
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	long nLines = fTEHandle[0]->nLines;
	if ((fTEHandle[0]->teLength <= 0) ||
		((*(CharsHandle)(fTEHandle[0]->hText))[fTEHandle[0]->teLength - 1] == ZKeyboard::ccReturn))
		++nLines;
	destRect.bottom = destRect.top + (fTEHandle[0]->lineHeight * nLines);
	ZRect viewRect = fTEHandle[0]->viewRect;
	ZPoint_T<double> extent(destRect.Width(), destRect.Height());
	ZPoint_T<double> translation(-destRect.Left(), -destRect.Top());
	ZPoint_T<double> size(viewRect.Width(), viewRect.Height());

	ZPoint_T<double> newScroll = translation/(extent - size) + delta;

	ZPoint newTranslation((extent.h - size.h)* newScroll.h, (extent.v - size.v)* newScroll.v);
	this->Internal_ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

void ZUITextEngine_TextEdit::ScrollTo(ZPoint_T<double> newValues, bool inDrawIt, bool inDoNotifications)
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	long nLines = fTEHandle[0]->nLines;
	if ((fTEHandle[0]->teLength <= 0) ||
		((*(CharsHandle)(fTEHandle[0]->hText))[fTEHandle[0]->teLength - 1] == ZKeyboard::ccReturn))
		++nLines;
	destRect.bottom = destRect.top + (fTEHandle[0]->lineHeight * nLines);
	ZRect viewRect = fTEHandle[0]->viewRect;
	ZPoint_T<double> extent(destRect.Width(), destRect.Height());
	ZPoint_T<double> translation(-destRect.Left(), -destRect.Top());
	ZPoint_T<double> size(viewRect.Width(), viewRect.Height());

	ZPoint newTranslation((extent.h - size.h)* newValues.h, (extent.v - size.v)* newValues.v);
	this->Internal_ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

void ZUITextEngine_TextEdit::ScrollStep(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications)
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	ZPoint newTranslation(-destRect.TopLeft());
	if (horizontal)
		{
		if (increase)
			newTranslation.h += 10;
		else
			newTranslation.h -= 10;
		}
	else
		{
		if (increase)
			newTranslation.v += fTEHandle[0]->lineHeight;
		else
			newTranslation.v -= fTEHandle[0]->lineHeight;
		}
	this->Internal_ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

void ZUITextEngine_TextEdit::ScrollPage(bool increase, bool horizontal, bool inDrawIt, bool inDoNotifications)
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	ZPoint newTranslation(-destRect.TopLeft());
	ZRect viewRect = fTEHandle[0]->viewRect;
	ZPoint delta(viewRect.Width() - 10, viewRect.Height() - fTEHandle[0]->lineHeight);
	if (horizontal)
		{
		if (increase)
			newTranslation.h += delta.h;
		else
			newTranslation.h -= delta.h;
		}
	else
		{
		if (increase)
			newTranslation.v += delta.v;
		else
			newTranslation.v -= delta.v;
		}
	this->Internal_ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

ZPoint_T<double> ZUITextEngine_TextEdit::GetScrollValues()
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	long nLines = fTEHandle[0]->nLines;
	if ((fTEHandle[0]->teLength <= 0) ||
		((*(CharsHandle)(fTEHandle[0]->hText))[fTEHandle[0]->teLength - 1] == ZKeyboard::ccReturn))
		++nLines;
	destRect.bottom = destRect.top + (fTEHandle[0]->lineHeight * nLines);
	ZRect viewRect = fTEHandle[0]->viewRect;
	ZPoint_T<double> extent(destRect.Width(), destRect.Height());
	ZPoint_T<double> translation(-destRect.Left(), -destRect.Top());
	ZPoint_T<double> size(viewRect.Width(), viewRect.Height());

	return translation/(extent - size);
	}

ZPoint_T<double> ZUITextEngine_TextEdit::GetScrollThumbProportions()
	{
	this->Internal_CheckGeometry();

	ZRect destRect = fTEHandle[0]->destRect;
	long nLines = fTEHandle[0]->nLines;
	if ((fTEHandle[0]->teLength <= 0) ||
		((*(CharsHandle)(fTEHandle[0]->hText))[fTEHandle[0]->teLength - 1] == ZKeyboard::ccReturn))
		++nLines;
	destRect.bottom = destRect.top + (fTEHandle[0]->lineHeight * nLines);
	ZRect viewRect = fTEHandle[0]->viewRect;
	ZPoint_T<double> extent(destRect.Width(), destRect.Height());
	ZPoint_T<double> size(viewRect.Width(), viewRect.Height());
	return size/extent;
	}

long ZUITextEngine_TextEdit::GetUserChangeCount()
	{ return fChangeCounter; }

void ZUITextEngine_TextEdit::SetFont(const ZDCFont& theFont)
	{
	ZDC theDCScratch = ZDCScratch::sGet();
	ZDCSetupForQD theSetupForQD(theDCScratch, true);
	::TextSize(theFont.GetSize());
	::TextFont(theFont.GetFontID());
	::TextFace(theFont.GetStyle());
	FontInfo theFontInfo;
	::GetFontInfo(&theFontInfo);

	fTEHandle[0]->txSize = theFont.GetSize();
	fTEHandle[0]->txFont = theFont.GetFontID();
	fTEHandle[0]->txFace = theFont.GetStyle();
	fTEHandle[0]->fontAscent = theFontInfo.ascent;
	fTEHandle[0]->lineHeight = theFontInfo.ascent + theFontInfo.descent + theFontInfo.leading;

	::TECalText(fTEHandle);
	}

size_t ZUITextEngine_TextEdit::PointToOffset(ZPoint inPoint)
	{
	this->Internal_CheckGeometry();
	return ::TEGetOffset(inPoint, fTEHandle);
	}

ZDCRgn ZUITextEngine_TextEdit::GetTextRgn(size_t inOffset, size_t inLength)
	{
	this->Internal_CheckGeometry();

	short oldStart = fTEHandle[0]->selStart;
	short oldEnd = fTEHandle[0]->selEnd;

	::TESetSelect(inOffset, inOffset + inLength, fTEHandle);
	ZDCRgn theRgn;
	::TEGetHiliteRgn(theRgn.GetRgnHandle(), fTEHandle);
	::TESetSelect(oldStart, oldEnd, fTEHandle);
	return theRgn;
	}

ZDCFont ZUITextEngine_TextEdit::GetFont()
	{ return ZDCFont(fTEHandle[0]->txFont, (ZDCFont::Style)fTEHandle[0]->txFace, fTEHandle[0]->txSize); }

void ZUITextEngine_TextEdit::SetTextColor(ZRGBColor textColor)
	{
	if (fTextColor == textColor)
		return;
	fTextColor = textColor;
	}
void ZUITextEngine_TextEdit::SetBackColor(ZRGBColor backColor)
	{
	if (fBackColor == backColor)
		return;
	fBackColor = backColor;
	}

ZRGBColor ZUITextEngine_TextEdit::GetTextColor()
	{ return fTextColor; }
ZRGBColor ZUITextEngine_TextEdit::GetBackColor()
	{ return fBackColor; }

void ZUITextEngine_TextEdit::Internal_ToggleDragCaret(const ZDC& inDC, size_t inDragOffset)
	{
	ZPoint locationOfDrag = ::TEGetPoint(fDragOffset, fTEHandle);
	TextStyle theStyle;
	short theLineHeight;
	short theFontAscent;
	::TEGetStyle(fDragOffset, &theStyle, &theLineHeight, &theFontAscent, fTEHandle);

	ZDC localDC(inDC);
	localDC.PenNormal();
	localDC.SetMode(ZDC::modeXor);
	localDC.Line(locationOfDrag.h - 1, locationOfDrag.v, locationOfDrag.h - 1, locationOfDrag.v - theLineHeight);
	}
void ZUITextEngine_TextEdit::Internal_ToggleDragBorder(const ZDC& inDC)
	{
	ZDCRgn borderRgn = this->GetHost()->GetHostPane(this)->CalcApertureRgn();
	ZUtil_Graphics::sDrawDragDropHilite(inDC, borderRgn);
	}
bool ZUITextEngine_TextEdit::Internal_CheckGeometry()
	{
	ZPoint mySize;
	ZCoord myWidth;
	fHost->GetEngineSizes(this, mySize, myWidth);
	if (mySize.h == (fTEHandle[0]->viewRect.right - fTEHandle[0]->viewRect.left) &&
		mySize.v == (fTEHandle[0]->viewRect.bottom - fTEHandle[0]->viewRect.left) &&
		myWidth == (fTEHandle[0]->destRect.right - fTEHandle[0]->destRect.left))
		return false;
	fTEHandle[0]->viewRect.left = 0;
	fTEHandle[0]->viewRect.right = mySize.h;
	fTEHandle[0]->viewRect.top = 0;
	fTEHandle[0]->viewRect.bottom = mySize.v;
	fTEHandle[0]->destRect.right = fTEHandle[0]->destRect.left + myWidth;
	::TECalText(fTEHandle);
	return true;
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
// =================================================================================================

static const char rcsid[] = "@(#) $Id: ZUI.cpp,v 1.10 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUI.h"
#include "ZMouseTracker.h"
#include "ZTextUtil.h"
#include "ZTicks.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer

void ZUICaptionRenderer::Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{
	ZCoord wrapWidth(inWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	localDC.SetTextColor(ZRGBColor::sBlack);
	localDC.SetFont(inFont);
	ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
	}

void ZUICaptionRenderer::Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{
	if (inDC.GetDepth() >= 4)
		inDC.Draw(inLocation, inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono(), inPixmapCombo.GetMask());
	else
		inDC.Draw(inLocation, inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor(), inPixmapCombo.GetMask());
	}

void ZUICaptionRenderer::DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{ this->Imp_DrawCaptionText(inDC, inLocation, inState, inText, inTextLength, inFont, inWrapWidth); }

void ZUICaptionRenderer::DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const string& inText, const ZDCFont& inFont, ZCoord inWrapWidth)
	{ this->Imp_DrawCaptionText(inDC, inLocation, inState, inText.data(), inText.size(), inFont, inWrapWidth); }

void ZUICaptionRenderer::DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{ this->Imp_DrawCaptionPixmap(inDC, inLocation, inState, inPixmapCombo); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption

ZUICaption::ZUICaption()
	{}

ZUICaption::~ZUICaption()
	{}

bool ZUICaption::IsValid()
	{ return true; }

bool ZUICaption::DrawsEntireRgn()
	{ return false; }

ZPoint ZUICaption::GetSize()
	{ return this->GetRgn().Bounds().BottomRight(); }

ZRect ZUICaption::GetBounds()
	{ return this->GetRgn().Bounds(); }

void ZUICaption::Draw(const ZDC& inDC, ZPoint inLocation, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZUIDisplayState dummyState;
	this->Draw(inDC, inLocation, dummyState, inCaptionRenderer);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Indirect

ZUICaption_Indirect::ZUICaption_Indirect(ZRef<ZUICaption> inRealCaption)
:	fRealCaption(inRealCaption)
	{}

ZUICaption_Indirect::~ZUICaption_Indirect()
	{}

bool ZUICaption_Indirect::IsValid()
	{
	if (fRealCaption)
		return fRealCaption->IsValid();
	return false;
	}

void ZUICaption_Indirect::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	if (fRealCaption)
		fRealCaption->Draw(inDC, inLocation, inState, inCaptionRenderer);
	}

bool ZUICaption_Indirect::DrawsEntireRgn()
	{
	if (fRealCaption)
		return fRealCaption->DrawsEntireRgn();
	return false;
	}

ZDCRgn ZUICaption_Indirect::GetRgn()
	{
	if (fRealCaption)
		return fRealCaption->GetRgn();
	return ZDCRgn();
	}

void ZUICaption_Indirect::SetRealCaption(ZRef<ZUICaption> inRealCaption)
	{ fRealCaption = inRealCaption; }

ZRef<ZUICaption> ZUICaption_Indirect::GetRealCaption()
	{ return fRealCaption; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Dynamic

ZUICaption_Dynamic::ZUICaption_Dynamic(Provider* inProvider)
:	fProvider(inProvider)
	{}

ZUICaption_Dynamic::~ZUICaption_Dynamic()
	{}

bool ZUICaption_Dynamic::IsValid()
	{
	if (fProvider)
		{
		if (ZRef<ZUICaption> realCaption = fProvider->ProvideUICaption(this))
			return realCaption->IsValid();
		}
	return false;
	}

void ZUICaption_Dynamic::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	if (fProvider)
		{
		if (ZRef<ZUICaption> realCaption = fProvider->ProvideUICaption(this))
			realCaption->Draw(inDC, inLocation, inState, inCaptionRenderer);
		}
	}

bool ZUICaption_Dynamic::DrawsEntireRgn()
	{
	if (fProvider)
		{
		if (ZRef<ZUICaption> realCaption = fProvider->ProvideUICaption(this))
			return realCaption->DrawsEntireRgn();
		}
	return false;
	}

ZDCRgn ZUICaption_Dynamic::GetRgn()
	{
	if (fProvider)
		{
		if (ZRef<ZUICaption> realCaption = fProvider->ProvideUICaption(this))
			return realCaption->GetRgn();
		}
	return ZDCRgn();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Text

ZUICaption_Text::ZUICaption_Text()
:	fChangeCount(0)
	{}

ZUICaption_Text::ZUICaption_Text(const string& inText, ZRef<ZUIFont> inUIFont, ZCoord inWrapWidth)
:	fText(inText), fUIFont(inUIFont), fWrapWidth(inWrapWidth), fChangeCount(inUIFont->GetChangeCount() - 1)
	{}

ZUICaption_Text::~ZUICaption_Text()
	{}

void ZUICaption_Text::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	if (inCaptionRenderer)
		inCaptionRenderer->DrawCaptionText(inDC, inLocation, inState, fText, fUIFont->GetFont(), fWrapWidth);
	else
		{
		ZCoord wrapWidth(fWrapWidth);
		if (wrapWidth <= 0)
			wrapWidth = 32767;

		ZDC localDC(inDC);
		localDC.SetTextColor(ZRGBColor::sBlack);
		localDC.SetFont(fUIFont->GetFont());
		//##ZTextUtil::sDraw(localDC, inLocation, wrapWidth, fText);
		}
	}

ZDCRgn ZUICaption_Text::GetRgn()
	{
	ZAssert(fUIFont);
	if (fChangeCount != fUIFont->GetChangeCount())
		{
		ZCoord wrapWidth(fWrapWidth);
		if (wrapWidth <= 0)
			wrapWidth = 32767;
		ZDC localDC(ZDCScratch::sGet());
		localDC.SetFont(fUIFont->GetFont());
		fCachedSize = ZTextUtil::sCalcSize(localDC, wrapWidth, fText);
		fChangeCount = fUIFont->GetChangeCount();
		}
	return ZRect(fCachedSize);
	}

void ZUICaption_Text::SetText(const string& inText)
	{
	--fChangeCount;
	fText = inText;
	}

const string& ZUICaption_Text::GetText()
	{ return fText; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_SimpleText

ZUICaption_SimpleText::ZUICaption_SimpleText()
:	fChangeCount(0), fTextColor(ZRGBColor::sBlack)
	{}

ZUICaption_SimpleText::ZUICaption_SimpleText(const string& inText, ZRef<ZUIFont> inUIFont, ZCoord inWrapWidth)
:	fText(inText), fTextColor(ZRGBColor::sBlack),
	fUIFont(inUIFont), fWrapWidth(inWrapWidth), fChangeCount(inUIFont->GetChangeCount() - 1)
	{}

ZUICaption_SimpleText::~ZUICaption_SimpleText()
	{}

void ZUICaption_SimpleText::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZCoord wrapWidth(fWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	localDC.SetTextColor(fTextColor); // colorize text captions. -ec 00.08.02
	localDC.SetFont(fUIFont->GetFont());
	//##ZTextUtil::sDraw(localDC, inLocation, wrapWidth, fText);
	}

ZDCRgn ZUICaption_SimpleText::GetRgn()
	{
	ZAssert(fUIFont);
	if (fChangeCount != fUIFont->GetChangeCount())
		{
		ZCoord wrapWidth(fWrapWidth);
		if (wrapWidth <= 0)
			wrapWidth = 32767;
		ZDC localDC(ZDCScratch::sGet());
		localDC.SetFont(fUIFont->GetFont());
		fCachedSize = ZTextUtil::sCalcSize(localDC, wrapWidth, fText);
		fChangeCount = fUIFont->GetChangeCount();
		}
	return ZRect(fCachedSize);
	}

void ZUICaption_SimpleText::SetText(const string& inText)
	{
	--fChangeCount;
	fText = inText;
	}

const string& ZUICaption_SimpleText::GetText()
	{ return fText; }

void ZUICaption_SimpleText::SetTextColor(const ZRGBColor& inTextColor)
	{
	if (fTextColor == inTextColor)
		return;
	fTextColor = inTextColor;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Pix

ZUICaption_Pix::ZUICaption_Pix()
	{}

ZUICaption_Pix::ZUICaption_Pix(const ZDCPixmap& inPixmap)
:	fPixmapCombo(inPixmap, ZDCPixmap(), ZDCPixmap())
	{}

ZUICaption_Pix::ZUICaption_Pix(const ZDCPixmapCombo& inPixmapCombo)
:	fPixmapCombo(inPixmapCombo)
	{}

ZUICaption_Pix::ZUICaption_Pix(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap)
:	fPixmapCombo(inColorPixmap, inMonoPixmap,inMaskPixmap)
	{}

ZUICaption_Pix::~ZUICaption_Pix()
	{}

void ZUICaption_Pix::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	if (inCaptionRenderer)
		inCaptionRenderer->DrawCaptionPixmap(inDC, inLocation, inState, fPixmapCombo);
	else
		{
		ZDCPixmap theColorPixmap;
		ZDCPixmap theMonoPixmap;
		ZDCPixmap theMaskPixmap;
		fPixmapCombo.GetPixmaps(theColorPixmap, theMonoPixmap, theMaskPixmap);
		if (inDC.GetDepth() >= 4)
			inDC.Draw(inLocation, theColorPixmap ? theColorPixmap : theMonoPixmap, theMaskPixmap);
		else
			inDC.Draw(inLocation, theMonoPixmap ? theMonoPixmap : theColorPixmap, theMaskPixmap);
		}
	}

bool ZUICaption_Pix::DrawsEntireRgn()
	{ return false; } //true; }

ZDCRgn ZUICaption_Pix::GetRgn()
	{ return ZRect(fPixmapCombo.Size()); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Composite_H

ZUICaption_Composite_H::ZUICaption_Composite_H()
	{}

ZUICaption_Composite_H::~ZUICaption_Composite_H()
	{}

void ZUICaption_Composite_H::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZCoord maxHeight = 0;
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		if (currentSize.v > maxHeight)
			maxHeight = currentSize.v;
		}
	ZPoint currentOrigin(inLocation);
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		(*i)->Draw(inDC, ZPoint(currentOrigin.h, currentOrigin.v + (maxHeight - currentSize.v)/2), inState, inCaptionRenderer);
		currentOrigin.h += currentSize.h;
		}
	}

ZDCRgn ZUICaption_Composite_H::GetRgn()
	{
	ZPoint theSize(ZPoint::sZero);
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		if (currentSize.v > theSize.v)
			theSize.v = currentSize.v;
		theSize.h += currentSize.h;
		}
	return ZRect(theSize);
	}

void ZUICaption_Composite_H::Add(ZRef<ZUICaption> theCaption)
	{ fCaptions.push_back(theCaption); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Composite_V

ZUICaption_Composite_V::ZUICaption_Composite_V()
	{}

ZUICaption_Composite_V::~ZUICaption_Composite_V()
	{}

void ZUICaption_Composite_V::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZCoord maxWidth = 0;
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		if (currentSize.h > maxWidth)
			maxWidth = currentSize.h;
		}
	ZPoint currentOrigin(inLocation);
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		(*i)->Draw(inDC, ZPoint(currentOrigin.h + (maxWidth - currentSize.h)/2, currentOrigin.v), inState, inCaptionRenderer);
		currentOrigin.v += currentSize.v;
		}
	}

ZDCRgn ZUICaption_Composite_V::GetRgn()
	{
	ZPoint theSize(ZPoint::sZero);
	for (vector<ZRef<ZUICaption> >::iterator i = fCaptions.begin(); i != fCaptions.end(); ++i)
		{
		ZPoint currentSize = (*i)->GetSize();
		if (currentSize.h > theSize.h)
			theSize.h = currentSize.h;
		theSize.v += currentSize.v;
		}
	return ZRect(theSize);
	}

void ZUICaption_Composite_V::Add(ZRef<ZUICaption> theCaption)
	{ fCaptions.push_back(theCaption); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionPane

ZUICaptionPane::ZUICaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUICaption> inCaption)
:	ZSubPane(inSuperPane, inPaneLocator), fCaption(inCaption)
	{}

ZUICaptionPane::~ZUICaptionPane()
	{}

void ZUICaptionPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fCaption)
		{
		ZDC localDC(inDC);
		if (fCaption->DrawsEntireRgn())
			localDC.SetClip(localDC.GetClip() - fCaption->GetRgn());
		ZSubPane::DoDraw(localDC, inBoundsRgn);
		ZUIDisplayState theState(false, this->GetHilite(), this->GetReallyEnabled(), false, this->GetActive());
		fCaption->Draw(inDC, ZPoint::sZero, theState, nil);
		}
	else
		ZSubPane::DoDraw(inDC, inBoundsRgn);
	}

ZPoint ZUICaptionPane::GetSize()
	{
	ZPoint mySize = ZPoint::sZero;
	bool gotSize = fPaneLocator && fPaneLocator->GetPaneSize(this, mySize);
	if (!gotSize)
		mySize = this->GetPreferredSize();
	return mySize;
	}

ZPoint ZUICaptionPane::GetPreferredSize()
	{
	if (fCaption)
		return fCaption->GetSize();
	return ZPoint::sZero;
	}

void ZUICaptionPane::SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh)
	{
	if (fCaption == inCaption)
		return;
	if (inRefresh)
		this->Refresh();
	fCaption = inCaption;
	if (inRefresh)
		this->Refresh();
	}

// Deprecated method
void ZUICaptionPane::SetCaptionWithRedraw(ZRef<ZUICaption> inCaption)
	{ this->SetCaption(inCaption, true); }

ZRef<ZUICaption> ZUICaptionPane::GetCaption()
	{ return fCaption; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIIconRenderer

ZUIIconRenderer::ZUIIconRenderer()
	{}

ZUIIconRenderer::~ZUIIconRenderer()
	{}

static bool sMungeProc_Darken(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor.red >>= 1;
	inOutColor.green >>= 1;
	inOutColor.blue >>= 1;
	return true;
	}

static bool sMungeProc_Lighten(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - ((ZRGBColor::sWhite - inOutColor) / 2);
	return true;
	}

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

// AG 98-12-07. Default implementation, draws icons using Apple's look.
void ZUIIconRenderer::Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled)
	{
	ZDC localDC(inDC);

	if (localDC.GetDepth() >= 4)
		{
		ZDCPixmap theSourcePixmap;
		if (inOpen)
			{
			if (inEnabled && inSelected)
				theSourcePixmap = sMakeHollowPixmap(inPixmapCombo.GetMask(), ZRGBColor::sBlack, ZRGBColor(0x3333, 0x3333, 0x8888));
			else
				{
				theSourcePixmap = sMakeHollowPixmap(inPixmapCombo.GetMask(), ZRGBColor::sBlack, ZRGBColor(0xCCCC, 0xCCCC, 0xFFFF));
				if (!inEnabled)
					theSourcePixmap.Munge(sMungeProc_Lighten, nil);
				}
			}
		else
			{
			theSourcePixmap = inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono();
			if (inEnabled)
				{
				if (inSelected)
					theSourcePixmap.Munge(sMungeProc_Darken, nil);
				}
			else
				theSourcePixmap.Munge(sMungeProc_Lighten, nil);

			}
		localDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	else
		{
		ZDCPixmap theSourcePixmap;
		if (inOpen)
			theSourcePixmap = sMakeHollowPixmap(inPixmapCombo.GetMask(), ZRGBColor::sBlack, ZRGBColor::sWhite);
		else
			theSourcePixmap = inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor();
		if (inSelected)
			theSourcePixmap.Munge(sMungeProc_Invert, nil);
		localDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	}

ZDCPixmap ZUIIconRenderer::sMakeHollowPixmap(const ZDCPixmap& inMask, const ZRGBColor& inForeColor, const ZRGBColor& inBackColor)
	{
	ZPoint theSize = inMask.Size();
	ZDCPixmap theHollowMask(theSize, ZDCPixmapNS::eFormatEfficient_Color_32, inBackColor);

	// First fill with the dithered pattern
	// Every other column
	for (ZCoord x = 0; x < theSize.h; x += 2)
		{
		ZCoord origin = 0;
		if ((x % 4) == 0)
			origin = 1;
		// Every other row
		for (ZCoord y = origin; y < theSize.v; y += 2)
			theHollowMask.SetPixel(x, y, inForeColor);
		}

	// Find edges
	bool wasInside = false;
	// Scan down each column, setting a pixel as we enter or leave the mask. Note that we rely on
	// ZDCPixmap::GetPixel returning black when we're outside the real bounds of the pixmap.
	for (ZCoord x = 0; x < theSize.h; ++x)
		{
		for (ZCoord y = 0; y <= theSize.v; ++y)
			{
			bool isInside = (inMask.GetPixel(x, y) == ZRGBColor::sWhite);
			if (isInside & !wasInside)
				theHollowMask.SetPixel(x, y, inForeColor);
			else if (!isInside && wasInside)
				theHollowMask.SetPixel(x, y - 1, inForeColor);
			wasInside = isInside;
			}
		}

	wasInside = false;
	// Now go across each row
	for (ZCoord y = 0; y < theSize.v; ++y)
		{
		for (ZCoord x = 0; x <= theSize.h; ++x)
			{
			bool isInside = (inMask.GetPixel(x, y) == ZRGBColor::sWhite);
			if (isInside & !wasInside)
				theHollowMask.SetPixel(x, y, inForeColor);
			else if (!isInside && wasInside)
				theHollowMask.SetPixel(x - 1, y, inForeColor);
			wasInside = isInside;
			}
		}

	return theHollowMask;
	}

ZRect ZUIIconRenderer::sCalcMaskBoundRect(const ZDCPixmap& inMask)
	{
	ZPoint theSize = inMask.Size();
	ZCoord hMin = theSize.h;
	ZCoord hMax = 0;
	ZCoord vMin = theSize.v;
	ZCoord vMax = 0;

	for (ZCoord y = 0; y < theSize.v; ++y)
		{
		for (ZCoord x = 0; x < theSize.h; ++x)
			{
			if (inMask.GetPixel(x, y) == ZRGBColor::sWhite)
				{
				hMin = min(hMin, x);
				hMax = max(hMax, x);
				vMin = min(vMin, y);
				vMax = max(vMax, y);
				}
			}
		}
	return ZRect(hMin, vMin, hMax + 1, vMax + 1);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIIconRenderer_Indirect

ZUIIconRenderer_Indirect::ZUIIconRenderer_Indirect(const ZRef<ZUIIconRenderer>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIIconRenderer_Indirect::~ZUIIconRenderer_Indirect()
	{}

void ZUIIconRenderer_Indirect::Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled)
	{
	if (ZRef<ZUIIconRenderer> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inDC, inLocation, inPixmapCombo, inSelected, inOpen, inEnabled);
	}

void ZUIIconRenderer_Indirect::SetRealRenderer(const ZRef<ZUIIconRenderer>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIIconRenderer> ZUIIconRenderer_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton

ZUIButton::ZUIButton(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, Responder* inResponder)
:	ZSubPane(inSuperPane, inPaneLocator), fResponder(inResponder)
	{}

ZUIButton::~ZUIButton()
	{}

void ZUIButton::Flash()
	{}

ZPoint ZUIButton::GetSize()
	{
	// We allow our PaneLocator to set our size, but also have the GetPreferredSize
	// method, which could also be invoked by our pane locator if it wants to override
	// only our height or width
	ZPoint theSize;
	if (fPaneLocator && fPaneLocator->GetPaneSize(this, theSize))
		return theSize;
	return this->CalcPreferredSize();
	}

ZPoint ZUIButton::GetPopupLocation()
	{ return ZPoint::sZero; }

ZPoint ZUIButton::CalcPreferredSize()
	{
	if (fSuperPane)
		return fSuperPane->GetInternalSize();
	return ZPoint::sZero;
	}

void ZUIButton::GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{ outTopLeftInset = outBottomRightInset = ZPoint::sZero; }

ZPoint ZUIButton::GetLocationInset()
	{ return this->GetLocation(); }

ZPoint ZUIButton::GetSizeInset()
	{ return this->GetSize(); }

void ZUIButton::SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh)
	{}

void ZUIButton::SetResponder(Responder* inResponder)
	{ fResponder = inResponder; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton::Responder

bool ZUIButton::Responder::HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent)
	{
	// Return true to abort tracking of the button
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton_Tracked::Tracker

class ZUIButton_Tracked::Tracker : public ZMouseTracker, public ZPaneLocator
	{
public:
	Tracker(ZUIButton_Tracked* inPane, ZPoint inStartPoint);
// From ZMouseTracker
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

// From ZPaneLocator
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);
	virtual void ReferencingSubPaneRemoved(ZSubPane* inSubPane);

private:
	ZUIButton_Tracked* fButtonPane;
	bool fPressed;
	bigtime_t fDownTime;
	};

// ==================================================

ZUIButton_Tracked::Tracker::Tracker(ZUIButton_Tracked* inPane, ZPoint inStartPoint)
:	ZMouseTracker(inPane, inStartPoint), fButtonPane(inPane), ZPaneLocator(inPane->GetPaneLocator()), fPressed(false)
	{}

void ZUIButton_Tracked::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	// AG 98-04-15. We take advantage of the ability to reconfigure pane locators on the fly
	// to insert ourselves as the button's pane locator whilst we're tracking -- we pass along
	// all requests to the previous locator except for inquiries as to whether the button is
	// pressed or not -- which we pull from our own instance variable
	if (!fButtonPane)
		return;

	ZRect myRect = fButtonPane->CalcBounds();
	bool newState = myRect.Contains(inNext);
	Responder* theResponder = fButtonPane->GetResponder();
	switch (inPhase)
		{
		case ePhaseBegin:
			fButtonPane->SetPaneLocator(this, false);
		case ePhaseContinue:
			if (newState != fPressed)
				{
				fPressed = newState;
				fButtonPane->DrawNow();
				if (theResponder)
					{
					if (newState)
						{
						fDownTime = ZTicks::sNow();
						if (theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonDown))
							this->Finish();
						}
					else
						{
						if (theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonUp))
							this->Finish();
						}
					}
				}
			else
				{
				if (theResponder)
					{
					if (newState)
						{
						if (ZTicks::sNow() - fDownTime > ZTicks::sDoubleClick())
							{
							fDownTime = ZTicks::sNow();
							if (theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonSqueezed))
								this->Finish();
							}
						else
							{
							if (theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonStillDown))
								this->Finish();
							}
						}
					else
						{
						if (theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonStillUp))
							this->Finish();
						}
					}
				}
			break;
		case ePhaseEnd:
			{
			if (fPressed)
				{
				if (theResponder)
					theResponder->HandleUIButtonEvent(fButtonPane, ZUIButton::ButtonAboutToBeReleasedWhileDown);
				ZUIButton_Tracked* theButtonPane = fButtonPane;
				fButtonPane = nil;
				theButtonPane->SetPaneLocator(fNextPaneLocator, false);
				fPressed = false;
				theButtonPane->DrawNow();
				if (theResponder)
					theResponder->HandleUIButtonEvent(theButtonPane, ZUIButton::ButtonReleasedWhileDown);
				}
			else
				{
				ZUIButton_Tracked* theButtonPane = fButtonPane;
				fButtonPane = nil;
				theButtonPane->SetPaneLocator(fNextPaneLocator, false);

				if (theResponder)
					theResponder->HandleUIButtonEvent(theButtonPane, ZUIButton::ButtonReleasedWhileUp);
				}
			}
			break;
		}
	}

bool ZUIButton_Tracked::Tracker::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inPane == fButtonPane && inAttribute == "ZUIButton::Pressed")
		{
		*((bool*)outResult) = fPressed;
		return true;
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

void ZUIButton_Tracked::Tracker::ReferencingSubPaneRemoved(ZSubPane* inSubPane)
	{
	if (inSubPane == fButtonPane)
		{
		ZDebugStopf(1, ("Oops. You deleted a button that's being tracked"));
		}
	ZPaneLocator::ReferencingSubPaneRemoved(inSubPane);
	}

// ==================================================
#pragma mark -
#pragma mark * ZUIButton_Tracked

ZUIButton_Tracked::ZUIButton_Tracked(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, Responder* inResponder)
:	ZUIButton(inSuperPane, inPaneLocator, inResponder)
	{}

ZUIButton_Tracked::~ZUIButton_Tracked()
	{}

void ZUIButton_Tracked::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
//##	ZDC_OffAuto localDC(inDC, false);
	ZSubPane::DoDraw(inDC, inBoundsRgn);
	this->PrivateDraw(inDC, inBoundsRgn, this->GetPressed(), this->GetHilite());
	}

bool ZUIButton_Tracked::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (inEvent.GetClickCount() == 2)
		{
		if (fResponder && fResponder->HandleUIButtonEvent(this, ButtonDoubleClick))
			return true;
		}

	Tracker* myTracker = new Tracker(this, inHitPoint);
	myTracker->Install();
	return true;
	}

void ZUIButton_Tracked::Flash()
	{
	if (this->GetReallyEnabled() && this->GetVisible())
		{
		{
		ZDCRgn boundsRgn;
		ZDC_OffAuto theOffDC(this->GetDC(boundsRgn), false);
		theOffDC.SetInk(this->GetRealBackInk(theOffDC));
		theOffDC.Fill(boundsRgn);
		this->PrivateDraw(theOffDC, boundsRgn, true, this->GetHilite());
		}

		if (fResponder)
			fResponder->HandleUIButtonEvent(this, ZUIButton::ButtonDown);

		// 1/4 second flash
		ZThread::sSleep(250);

		if (fResponder)
			fResponder->HandleUIButtonEvent(this, ZUIButton::ButtonAboutToBeReleasedWhileDown);

		{
		ZDCRgn boundsRgn;
		ZDC_OffAuto theOffDC(this->GetDC(boundsRgn), false);
		theOffDC.SetInk(this->GetRealBackInk(theOffDC));
		theOffDC.Fill(boundsRgn);
		this->PrivateDraw(theOffDC, boundsRgn, false, this->GetHilite());
		}

		if (fResponder)
			fResponder->HandleUIButtonEvent(this, ZUIButton::ButtonReleasedWhileDown);
		}
	}

bool ZUIButton_Tracked::GetPressed()
	{
	bool thePressed(false);
	if (this->GetAttribute("ZUIButton::Pressed", nil, &thePressed))
		return thePressed;
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRange

ZUIRange::ZUIRange()
	{}

ZUIRange::~ZUIRange()
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->RangeGone(this);
	}

void ZUIRange::AddResponder(Responder* inResponder)
	{
	ZAssertStop(2, inResponder);
	fResponders.push_back(inResponder);
	}

void ZUIRange::RemoveResponder(Responder* inResponder)
	{
	vector<Responder*>::iterator theIter = find(fResponders.begin(), fResponders.end(), inResponder);
	ZAssertStop(2, theIter != fResponders.end());
	fResponders.erase(theIter);
	}

void ZUIRange::Internal_NotifyResponders(Event inEvent, Part inRangePart)
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->HandleUIRangeEvent(this, inEvent, inRangePart);
	}

void ZUIRange::Internal_NotifyResponders(double inNewvalue)
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->HandleUIRangeSliderEvent(this, inNewvalue);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRange::Responder

void ZUIRange::Responder::RangeGone(ZUIRange* inRange)
	{}

bool ZUIRange::Responder::HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart)
	{ return false; }

bool ZUIRange::Responder::HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue)
	{ return false; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollBar

ZUIScrollBar::ZUIScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator)
	{}

ZUIScrollBar::~ZUIScrollBar()
	{}

double ZUIScrollBar::GetValue()
	{
	double theValue;
	if (this->GetAttribute("ZUIRange::Value", nil, &theValue))
		return theValue;
	return 0.0;
	}

double ZUIScrollBar::GetThumbProportion()
	{
	// Proportion should be between 0.0 and 1.0. If you return outside that range, most scrollbars
	// will clamp the value appropriately. A thumb proportion of 1.0 (or greater) indicates
	// non-scrollability.
	double theValue;
	if (this->GetAttribute("ZUIScrollBar::ThumbProportion", nil, &theValue))
		return theValue;
	return 0.0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISlider

ZUISlider::ZUISlider(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator)
	{}

ZUISlider::~ZUISlider()
	{}

double ZUISlider::GetValue()
	{
	double theValue;
	if (this->GetAttribute("ZUIRange::Value", nil, &theValue))
		return theValue;
	return 0.0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUILittleArrows

ZUILittleArrows::ZUILittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator)
	{}

ZUILittleArrows::~ZUILittleArrows()
	{}

// From ZUIRange
double ZUILittleArrows::GetValue()
	{ return 0.0; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIProgressBar

ZUIProgressBar::ZUIProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator), fNextTick(0)
	{
	this->GetWindow()->RegisterIdlePane(this);
	}

ZUIProgressBar::~ZUIProgressBar()
	{
	this->GetWindow()->UnregisterIdlePane(this);
	}

void ZUIProgressBar::DoIdle()
	{
	if (fNextTick <= ZTicks::sNow())
		{
		if (this->GetReallyVisible())
			{
			bigtime_t waitDuration = this->NextFrame();
			fNextTick = ZTicks::sNow() + waitDuration;
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIChasingArrows

ZUIChasingArrows::ZUIChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator), fNextTick(0)
	{
	this->GetWindow()->RegisterIdlePane(this);
	}

ZUIChasingArrows::~ZUIChasingArrows()
	{
	this->GetWindow()->UnregisterIdlePane(this);
	}

void ZUIChasingArrows::DoIdle()
	{
	if (fNextTick <= ZTicks::sNow())
		{
		bigtime_t waitDuration = this->NextFrame();
		fNextTick = ZTicks::sNow() + waitDuration;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAlternatePane

ZUIAlternatePane::ZUIAlternatePane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSuperPane(inSuperPane, inPaneLocator)
	{}

ZUIAlternatePane::~ZUIAlternatePane()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITabPane

ZUITabPane::ZUITabPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZUIAlternatePane(inSuperPane, inPaneLocator)
	{}

ZUITabPane::~ZUITabPane()
	{}

void ZUITabPane::SetCurrentTab(ZSuperPane* inPane)
	{ this->SetCurrentPane(inPane); }

ZSuperPane* ZUITabPane::GetCurrentTab()
	{ return this->GetCurrentPane(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGroup

ZUIGroup::ZUIGroup(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSuperPane(inSuperPane, inPaneLocator)
	{}

ZUIGroup::~ZUIGroup()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISelection

ZUISelection::ZUISelection()
	{}

ZUISelection::~ZUISelection()
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->SelectionGone(this);
	}

void ZUISelection::AddResponder(Responder* inResponder)
	{
	ZAssertStop(2, inResponder);
	fResponders.push_back(inResponder);
	}

void ZUISelection::RemoveResponder(Responder* inResponder)
	{
	vector<Responder*>::iterator theIter = find(fResponders.begin(), fResponders.end(), inResponder);
	ZAssertStop(2, theIter != fResponders.end());
	fResponders.erase(theIter);
	}

void ZUISelection::Internal_NotifyResponders(Event inEvent)
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->HandleUISelectionEvent(this, inEvent);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISelection::Responder

void ZUISelection::Responder::SelectionGone(ZUISelection* inSelection)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollable

ZUIScrollable::ZUIScrollable()
	{}

ZUIScrollable::~ZUIScrollable()
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->ScrollableGone(this);
	}

void ZUIScrollable::AddResponder(Responder* inResponder)
	{
	ZAssertStop(2, inResponder);
	fResponders.push_back(inResponder);
	}

void ZUIScrollable::RemoveResponder(Responder* inResponder)
	{
	vector<Responder*>::iterator theIter = find(fResponders.begin(), fResponders.end(), inResponder);
	ZAssertStop(2, theIter != fResponders.end());
	fResponders.erase(theIter);
	}

void ZUIScrollable::Internal_NotifyResponders()
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->ScrollableChanged(this);
	}

bool ZUIScrollable::DoCursorKey(const ZEvent_Key& inEvent)
	{
	ZPoint_T<double> currentScroll = this->GetScrollValues();
	ZPoint_T<double> currentThumbProportions = this->GetScrollThumbProportions();
	switch (inEvent.GetCharCode())
		{
		case ZKeyboard::ccUp:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 0.0)
				{
				this->ScrollStep(false, false, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccDown:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 1.0)
				{
				this->ScrollStep(true, false, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccLeft:
			{
			if (currentThumbProportions.h < 1.0 && currentScroll.h != 0.0)
				{
				this->ScrollStep(false, true, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccRight:
			{
			if (currentThumbProportions.h < 1.0 && currentScroll.h != 1.0)
				{
				this->ScrollStep(true, true, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccPageUp:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 0.0)
				{
				this->ScrollPage(false, false, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccPageDown:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 1.0)
				{
				this->ScrollPage(true, false, true, true);
				return true;
				}
			}
			break;
		case ZKeyboard::ccHome:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 0.0)
				{
				this->ScrollTo(ZPoint_T<double>(currentScroll.h, 0.0), true, true);
				return true;
				}
			else
				{
				if (currentThumbProportions.h < 1.0 && currentScroll.h != 0.0)
					{
					this->ScrollTo(ZPoint_T<double>(0.0, 0.0), true, true);
					return true;
					}
				}
			}
			break;
		case ZKeyboard::ccEnd:
			{
			if (currentThumbProportions.v < 1.0 && currentScroll.v != 1.0)
				{
				this->ScrollTo(ZPoint_T<double>(currentScroll.h, 1.0), true, true);
				return true;
				}
			else
				{
				if (currentThumbProportions.h < 1.0 && currentScroll.h != 1.0)
					{
					this->ScrollTo(ZPoint_T<double>(1.0, 1.0), true, true);
					return true;
					}
				}
			}
			break;
		}
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollable::Responder

void ZUIScrollable::Responder::ScrollableGone(ZUIScrollable* inScrollable)
	{}

void ZUIScrollable::Responder::ScrollableChanged(ZUIScrollable* inScrollable)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITextContainer

ZUITextContainer::ZUITextContainer()
	{}

ZUITextContainer::~ZUITextContainer()
	{}

string ZUITextContainer::GetText(size_t inOffset, size_t inLength)
	{
	if (inLength == 0)
		return string();
	string aString(inLength, ' ');
	size_t outLength;
	this->GetText_Impl(&aString[0], inOffset, inLength, outLength);
	if (outLength != inLength)
		aString.resize(outLength);
	return aString;
	}

void ZUITextContainer::GetText(char* outDest, size_t inOffset, size_t inLength, size_t& outLength)
	{ this->GetText_Impl(outDest, inOffset, inLength, outLength); }

string ZUITextContainer::GetText()
	{ return this->GetText(0, this->GetTextLength()); }

char ZUITextContainer::GetChar(size_t inOffset)
	{
	char theChar;
	size_t realLength;
	this->GetText_Impl(&theChar, inOffset, 1, realLength);
	if (realLength == 1)
		return theChar;
	return 0;
	}

void ZUITextContainer::SetText(const char* inSource, size_t inLength)
	{ this->SetText_Impl(inSource, inLength); }

void ZUITextContainer::SetText(const string& inText)
	{
	if (inText.size())
		this->SetText_Impl(&inText.at(0), inText.size());
	else
		this->SetText_Impl(nil, 0);
	}

void ZUITextContainer::InsertText(size_t inInsertOffset, const char* inSource, size_t inLength)
	{ this->InsertText_Impl(inInsertOffset, inSource, inLength); }

void ZUITextContainer::InsertText(size_t inInsertOffset, const string& inText)
	{
	if (inText.size() > 0)
		this->InsertText_Impl(inInsertOffset, inText.data(), inText.size());
	}

void ZUITextContainer::ReplaceText(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength)
	{ this->ReplaceText_Impl(inReplaceOffset, inReplaceLength, inSource, inLength); }

void ZUITextContainer::ReplaceText(size_t inReplaceOffset, size_t inReplaceLength, const string& inText)
	{
	if (inText.size() > 0)
		this->ReplaceText_Impl(inReplaceOffset, inReplaceLength, inText.data(), inText.size());
	else
		{
		if (inReplaceLength > 0)
			this->ReplaceText_Impl(inReplaceOffset, inReplaceLength, nil, 0);
		}
	}

void ZUITextContainer::ReplaceSelection_Impl(const char* inSource, size_t inLength)
	{
	size_t selectionStart, selectionLength;
	this->GetSelection(selectionStart, selectionLength);
	this->ReplaceText_Impl(selectionStart, selectionLength, inSource, inLength);
	}

void ZUITextContainer::ReplaceSelection(const char* inSource, size_t inLength)
	{ this->ReplaceSelection_Impl(inSource, inLength); }

void ZUITextContainer::ReplaceSelection(const string& inText)
	{
	if (inText.size())
		this->ReplaceSelection_Impl(inText.data(), inText.size());
	else
		this->ReplaceSelection_Impl(nil, 0);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITextPane

ZUITextPane::ZUITextPane(ZSuperPane* superPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(superPane, inPaneLocator)
	{}

ZUITextPane::~ZUITextPane()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFocusFrame

ZUIFocusFrame::ZUIFocusFrame(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSuperPane(inSuperPane, inPaneLocator)
	{}

ZUIFocusFrame::~ZUIFocusFrame()
	{}

ZDCRgn ZUIFocusFrame::GetBorderRgn()
	{
	ZRect myBounds = this->CalcBounds();
	ZPoint topLeftInset, bottomRightInset;
	this->GetInsets(topLeftInset, bottomRightInset);
	ZRect insetBounds(myBounds);
	insetBounds.left += topLeftInset.h;
	insetBounds.top += topLeftInset.v;
	insetBounds.right += bottomRightInset.h;
	insetBounds.bottom += bottomRightInset.v;
	return ZDCRgn(myBounds) ^ ZDCRgn(insetBounds);
	}

bool ZUIFocusFrame::GetBorderShown()
	{ return this->IsParentOf(this->GetWindowTarget()); }

void ZUIFocusFrame::HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	ZSuperPane::HandlerBecameWindowTarget(inOldTarget, inNewTarget);
	this->Invalidate(this->GetBorderRgn());
	}

void ZUIFocusFrame::HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	this->Invalidate(this->GetBorderRgn());
	ZSuperPane::HandlerResignedWindowTarget(inOldTarget, inNewTarget);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISplitter

ZUISplitter::ZUISplitter(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSuperPane(inSuperPane, inPaneLocator)
	{}

ZUISplitter::~ZUISplitter()
	{}

ZSuperPane* ZUISplitter::GetPane1()
	{
	ZSuperPane* pane1;
	ZSuperPane* pane2;
	this->GetPanes(pane1, pane2);
	return pane1;
	}

ZSuperPane* ZUISplitter::GetPane2()
	{
	ZSuperPane* pane1;
	ZSuperPane* pane2;
	this->GetPanes(pane1, pane2);
	return pane2;
	}

bool ZUISplitter::GetOrientation()
	{
// true == horizontal
	bool orientation;
	if (this->GetAttribute("ZUISplitter::Orientation", nil, &orientation))
		return orientation;
	return true;
	}

void ZUISplitter::AddResponder(Responder* inResponder)
	{
	ZAssertStop(2, inResponder);
	fResponders.push_back(inResponder);
	}

void ZUISplitter::RemoveResponder(Responder* inResponder)
	{
	vector<Responder*>::iterator theIter = find(fResponders.begin(), fResponders.end(), inResponder);
	ZAssertStop(2, theIter != fResponders.end());
	fResponders.erase(theIter);
	}

void ZUISplitter::Internal_NotifyResponders(ZCoord inNewPane1Size, ZCoord inNewPane2Size)
	{
	for (vector<Responder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->SplitterChanged(this, inNewPane1Size, inNewPane2Size);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISplitter::Responder

void ZUISplitter::Responder::SplitterGone(ZUISplitter* inSplitter)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIDivider

ZUIDivider::ZUIDivider(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator)
	{}

ZUIDivider::~ZUIDivider()
	{}

bool ZUIDivider::GetOrientation()
	{
// true == horizontal
	bool orientation;
	if (this->GetAttribute("ZUIDivider::Orientation", nil, &orientation))
		return orientation;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollPane

ZUIScrollPane::ZUIScrollPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSuperPane(inSuperPane, inPaneLocator), fTranslation(ZPoint::sZero)
	{}

ZUIScrollPane::~ZUIScrollPane()
	{}

ZPoint ZUIScrollPane::GetTranslation()
	{ return fTranslation; }

bool ZUIScrollPane::DoKeyDown(const ZEvent_Key& inEvent)
	{
	// Call our ZUIScrollable utility method
	if (this->DoCursorKey(inEvent))
		return true;
	return ZSuperPane::DoKeyDown(inEvent);
	}

void ZUIScrollPane::BringPointIntoViewPinned(ZPoint inPosition)
	{ this->BringPointIntoView(inPosition); }

void ZUIScrollPane::BringPointIntoView(ZPoint inPosition)
	{
	// Get our current aperture
	ZRect apertureRect = this->CalcAperture();

	ZPoint theDelta = ZPoint::sZero;
	if (inPosition.h < apertureRect.left)
		theDelta.h = inPosition.h - apertureRect.left;
	else if (inPosition.h >= apertureRect.right)
		theDelta.h = inPosition.h - apertureRect.right;

	if (inPosition.v < apertureRect.top)
		theDelta.v = inPosition.v - apertureRect.top;
	else if (inPosition.v >= apertureRect.bottom)
		theDelta.v = inPosition.v - apertureRect.bottom;

	if (theDelta != ZPoint::sZero)
		this->ScrollBy(theDelta, true, true);
	/*! \todo Need to figure out how many pixels we were able to move, and pass the
	call up the chain if we weren't able to do the whole job */
	}

bool ZUIScrollPane::WantsToBecomeTarget()
	{ return ZSuperPane::WantsToBecomeTarget(); }

void ZUIScrollPane::ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint mySize = this->GetSize();
	ZPoint subPaneExtent = this->GetSubPaneExtent();

	ZRect shownExtent;
	shownExtent.left = min((ZCoord)0, fTranslation.h);
	shownExtent.top = min((ZCoord)0, fTranslation.v);
	shownExtent.right = max(subPaneExtent.h, (ZCoord)(mySize.h + fTranslation.h));
	shownExtent.bottom = max(subPaneExtent.v, (ZCoord)(mySize.v + fTranslation.v));

	ZPoint integerDelta;
	integerDelta.h = static_cast<ZCoord>(inDelta.h * (shownExtent.Width() - mySize.h));
	integerDelta.v = static_cast<ZCoord>(inDelta.v * (shownExtent.Height() - mySize.v));

	this->ScrollTo(fTranslation + integerDelta, inDrawIt, inDoNotifications);
	}

void ZUIScrollPane::ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint mySize = this->GetSize();
	ZPoint subPaneExtent = this->GetSubPaneExtent();

	ZRect shownExtent;
	shownExtent.left = min((ZCoord)0, fTranslation.h);
	shownExtent.top = min((ZCoord)0, fTranslation.v);
	shownExtent.right = max(subPaneExtent.h, (ZCoord)(mySize.h + fTranslation.h));
	shownExtent.bottom = max(subPaneExtent.v, (ZCoord)(mySize.v + fTranslation.v));

	ZPoint newTranslation;
	newTranslation.h = static_cast<ZCoord>(inNewValues.h * (shownExtent.Width() - mySize.h));
	newTranslation.v = static_cast<ZCoord>(inNewValues.v * (shownExtent.Height() - mySize.v));

	this->ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

void ZUIScrollPane::ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint scrollStep;
	if (!this->GetAttribute("ZUIScrollPane::ScrollStep", nil, &scrollStep))
		scrollStep = ZPoint(10, 10);
	ZPoint theDelta(ZPoint::sZero);
	if (inHorizontal)
		theDelta.h = scrollStep.h;
	else
		theDelta.v = scrollStep.v;
	if (!inIncrease)
		theDelta *= -1;

	this->ScrollTo(fTranslation + theDelta, inDrawIt, inDoNotifications);
	}

void ZUIScrollPane::ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint scrollOverlap;
	if (!this->GetAttribute("ZUIScrollPane::ScrollOverlap", nil, &scrollOverlap))
		scrollOverlap = ZPoint(10, 10);
	ZPoint theDelta(ZPoint::sZero);
	if (inHorizontal)
		theDelta.h = this->GetSize().h - scrollOverlap.h;
	else
		theDelta.v = this->GetSize().v - scrollOverlap.v;
	if (!inIncrease)
		theDelta *= -1;

	this->ScrollTo(fTranslation + theDelta, inDrawIt, inDoNotifications);
	}

ZPoint_T<double> ZUIScrollPane::GetScrollValues()
	{
	ZPoint subPaneExtent = this->GetSubPaneExtent();
	ZPoint mySize(this->GetSize());

	ZRect shownExtent;
	shownExtent.left = min((ZCoord)0, fTranslation.h);
	shownExtent.top = min((ZCoord)0, fTranslation.v);
	shownExtent.right = max(subPaneExtent.h, (ZCoord)(mySize.h + fTranslation.h));
	shownExtent.bottom = max(subPaneExtent.v, (ZCoord)(mySize.v + fTranslation.v));

	ZPoint_T<double> translation(fTranslation.h, fTranslation.v);
	ZPoint_T<double> extent(shownExtent.Width(), shownExtent.Height());
	ZPoint_T<double> size(mySize.h, mySize.v);

	double scrollValueH = translation.h/(extent.h - size.h);
	if (extent.h == size.h)
		scrollValueH = 0.0;
	double scrollValueV = translation.v/(extent.v - size.v);
	if (extent.v == size.v)
		scrollValueV = 0.0;
	ZPoint_T<double>(scrollValueH, scrollValueV);
	return translation/(extent - size);
	}

ZPoint_T<double> ZUIScrollPane::GetScrollThumbProportions()
	{
	ZPoint subPaneExtent = this->GetSubPaneExtent();
	ZPoint mySize(this->GetSize());
	ZRect shownExtent;
	shownExtent.left = min((ZCoord)0, fTranslation.h);
	shownExtent.top = min((ZCoord)0, fTranslation.v);
	shownExtent.right = max(subPaneExtent.h, (ZCoord)(mySize.h + fTranslation.h));
	shownExtent.bottom = max(subPaneExtent.v, (ZCoord)(mySize.v + fTranslation.v));

	ZPoint_T<double> extent(shownExtent.Width(), shownExtent.Height());
	ZPoint_T<double> size(mySize.h, mySize.v);

	return size/extent;
	}

void ZUIScrollPane::ScrollBy(ZPoint inDelta, bool inDrawIt, bool inDoNotifications)
	{
	this->ScrollTo(fTranslation + inDelta, inDrawIt, inDoNotifications);
	}

void ZUIScrollPane::ScrollTo(ZPoint inNewTranslation, bool inDrawIt, bool inDoNotifications)
	{
	// AG 98-05-12. For the moment pin the translation values so
	// we don't get weird scrolling. Not the final answer, as we want
	// to allow negative and greater-than extent scrolling.

	ZPoint mySize(this->GetSize());
	ZPoint subPaneExtent = this->GetSubPaneExtent();

	ZRect shownExtent;
	shownExtent.left = min((ZCoord)0, fTranslation.h);
	shownExtent.top = min((ZCoord)0, fTranslation.v);
	shownExtent.right = max(subPaneExtent.h, (ZCoord)(mySize.h + fTranslation.h));
	shownExtent.bottom = max(subPaneExtent.v, (ZCoord)(mySize.v + fTranslation.v));

	ZPoint maxOffset;
	maxOffset.h = shownExtent.Width() - mySize.h;
	maxOffset.v = shownExtent.Height() - mySize.v;

	ZPoint minOffset;
	minOffset.h = min((ZCoord)0, fTranslation.h);
	minOffset.v = min((ZCoord)0, fTranslation.v);

	inNewTranslation.h = max(minOffset.h, min(inNewTranslation.h, maxOffset.h));
	inNewTranslation.v = max(minOffset.v, min(inNewTranslation.v, maxOffset.v));

	this->Internal_ScrollTo(inNewTranslation, inDrawIt, inDoNotifications);
	}

ZPoint ZUIScrollPane::GetSubPaneExtent()
	{
	ZPoint extentMax(ZPoint::sZero);
	for (vector<ZSubPane*>::iterator i = fSubPanes.begin(); i != fSubPanes.end(); ++i)
		{
		ZPoint currentExtent = (*i)->GetSize() + (*i)->GetLocation();
		if (currentExtent.h > extentMax.h)
			extentMax.h = currentExtent.h;
		if (currentExtent.v > extentMax.v)
			extentMax.v = currentExtent.v;
		}
	return extentMax;
	}

void ZUIScrollPane::Internal_ScrollTo(ZPoint inNewTranslation, bool inDrawIt, bool inDoNotifications)
	{
	// Only do stuff if we've changed translations
	if (fTranslation != inNewTranslation)
		{
		this->ScrollPreChange();

		ZPoint oldTranslation = fTranslation;
		fTranslation = inNewTranslation;

		// And do the scroll if we were asked to move the bits
		if (inDrawIt)
			this->GetDC().Scroll(this->CalcVisibleBoundsRgn().Bounds(), oldTranslation - inNewTranslation);

		this->ScrollPostChange();

		if (inDoNotifications)
			this->Internal_NotifyResponders();

		if (inDrawIt)
			this->Update();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollableScrollableConnector

ZUIScrollableScrollableConnector::ZUIScrollableScrollableConnector(ZUIScrollable* scrollable1, ZUIScrollable* scrollable2, bool slaveHorizontal)
:	fScrollable1(scrollable1), fScrollable2(scrollable2), fSlaveHorizontal(slaveHorizontal)
	{
	ZAssertStop(2, fScrollable1);
	ZAssertStop(2, fScrollable2);
	fScrollable1->AddResponder(this);
	fScrollable2->AddResponder(this);
	}

ZUIScrollableScrollableConnector::~ZUIScrollableScrollableConnector()
	{
	if (fScrollable1)
		fScrollable1->RemoveResponder(this);
	if (fScrollable2)
		fScrollable2->RemoveResponder(this);
	}

void ZUIScrollableScrollableConnector::ScrollableChanged(ZUIScrollable* inScrollable)
	{
	ZPoint_T<double> scrollable1Values = fScrollable1->GetScrollValues();
	ZPoint_T<double> scrollable2Values = fScrollable2->GetScrollValues();
	if (inScrollable == fScrollable1)
		{
		ZPoint_T<double> newScrollable2values;
		if (fSlaveHorizontal)
			{
			newScrollable2values.h = scrollable1Values.h;
			newScrollable2values.v = scrollable2Values.v;
			}
		else
			{
			newScrollable2values.h = scrollable2Values.h;
			newScrollable2values.v = scrollable1Values.v;
			}
		fScrollable2->ScrollTo(newScrollable2values, true, false);
		}
	else if (inScrollable == fScrollable2)
		{
		ZPoint_T<double> newScrollable1values;
		if (fSlaveHorizontal)
			{
			newScrollable1values.h = scrollable2Values.h;
			newScrollable1values.v = scrollable1Values.v;
			}
		else
			{
			newScrollable1values.h = scrollable1Values.h;
			newScrollable1values.v = scrollable2Values.v;
			}
		fScrollable1->ScrollTo(scrollable1Values, true, false);
		}
	else
		{
		ZDebugLogf(1, ("ZUIScrollableScrollableConnector::ScrollableChanged, didn't recognize the scrollable"));
		}
	}

void ZUIScrollableScrollableConnector::ScrollableGone(ZUIScrollable* inScrollable)
	{
	if (inScrollable == fScrollable1)
		fScrollable1 = nil;
	else if (inScrollable == fScrollable2)
		fScrollable2 = nil;
	else
		{
		ZDebugStopf(1, ("ZUIScrollableScrollableConnector::ScrollableGone, didn't recognize the scrollable"));
		}
	delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollableScrollBarConnector

ZUIScrollableScrollBarConnector::ZUIScrollableScrollBarConnector(ZUIScrollBar* inScrollBar, ZUIScrollable* inScrollable, ZSubPane* inAdjacentPane, bool inHorizontal)
:	ZPaneLocatorSimple(inScrollBar->GetPaneLocator()),
	fScrollBar(inScrollBar),
	fScrollable(inScrollable),
	fAdjacentPane(inAdjacentPane),
	fHorizontal(inHorizontal), fTopOffset(0), fBottomOffset(0)
	{
	ZAssertStop(2, fScrollBar);
	ZAssertStop(2, fScrollable);
	fScrollBar->SetPaneLocator(this, false);
	fScrollBar->AddResponder(this);
	fScrollable->AddResponder(this);
	}

ZUIScrollableScrollBarConnector::ZUIScrollableScrollBarConnector(ZUIScrollBar* inScrollBar, ZUIScrollable* inScrollable, ZSubPane* inAdjacentPane, bool inHorizontal, ZCoord inTopOffset, ZCoord inBottomOffset)
:	ZPaneLocatorSimple(inScrollBar->GetPaneLocator()),
	fScrollBar(inScrollBar),
	fScrollable(inScrollable),
	fAdjacentPane(inAdjacentPane),
	fHorizontal(inHorizontal), fTopOffset(inTopOffset), fBottomOffset(inBottomOffset)
	{
	ZAssertStop(2, fScrollBar);
	ZAssertStop(2, fScrollable);
	fScrollBar->SetPaneLocator(this, false);
	fScrollBar->AddResponder(this);
	fScrollable->AddResponder(this);
	}

ZUIScrollableScrollBarConnector::~ZUIScrollableScrollBarConnector()
	{
	if (fScrollBar)
		fScrollBar->RemoveResponder(this);
	if (fScrollable)
		fScrollable->RemoveResponder(this);
	}

void ZUIScrollableScrollBarConnector::ScrollableChanged(ZUIScrollable* inScrollable)
	{
	fScrollBar->Refresh();
	}

void ZUIScrollableScrollBarConnector::RangeGone(ZUIRange* inRange)
	{
	if (inRange == fScrollBar)
		{
		fScrollBar = nil;
		delete this;
		}
	}
void ZUIScrollableScrollBarConnector::ScrollableGone(ZUIScrollable* inScrollable)
	{
	if (inScrollable == fScrollable)
		{
		fScrollable = nil;
		delete this;
		}
	}

// From ZUIRange::Responder
bool ZUIScrollableScrollBarConnector::HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart)
	{
	if (inEvent == ZUIRange::EventDown || inEvent == ZUIRange::EventStillDown)
		{
		if (inRangePart == ZUIRange::PartStepUp)
			fScrollable->ScrollStep(false, fHorizontal, true, true);
		else if (inRangePart == ZUIRange::PartStepDown)
			fScrollable->ScrollStep(true, fHorizontal, true, true);
		else if (inRangePart == ZUIRange::PartPageUp)
			fScrollable->ScrollPage(false, fHorizontal, true, true);
		else if (inRangePart == ZUIRange::PartPageDown)
			fScrollable->ScrollPage(true, fHorizontal, true, true);
		}
	return false;
	}

bool ZUIScrollableScrollBarConnector::HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue)
	{
	ZPoint_T<double> currentValues = fScrollable->GetScrollValues();
	if (fHorizontal)
		currentValues.h = inNewValue;
	else
		currentValues.v = inNewValue;
	fScrollable->ScrollTo(currentValues, true, true);
	return false;
	}

// From ZPaneLocator
bool ZUIScrollableScrollBarConnector::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	// AG 98-06-24. Do not overlap scrollbars with windows edges -- that's the scroll bar's problem
	if (inPane == fScrollBar)
		{
		if (fAdjacentPane)
			{
			ZPoint adjacentPaneSize(fAdjacentPane->GetSize());
			// Work in adjacentPane's superpane's coordinate system -- otherwise the superpane's translation kicks in
			ZPoint adjacentPaneWindowLocation(fAdjacentPane->GetSuperPane()->ToWindow(fAdjacentPane->GetLocation()));
			ZPoint scrollBarLocation(fScrollBar->GetSuperPane()->FromWindow(adjacentPaneWindowLocation));
			if (fHorizontal)
				{
				outLocation.v = scrollBarLocation.v + adjacentPaneSize.v;
				outLocation.h = scrollBarLocation.h + fTopOffset;
				}
			else
				{
				outLocation.v = scrollBarLocation.v + fTopOffset;
				outLocation.h = scrollBarLocation.h + adjacentPaneSize.h;
				}
			return true;
			}
		}
	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool ZUIScrollableScrollBarConnector::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (inPane == fScrollBar)
		{
		if (fAdjacentPane)
			{
			ZPoint adjacentPaneSize(fAdjacentPane->GetSize());
			ZCoord thickness = fScrollBar->GetPreferredThickness();
			if (fHorizontal)
				{
				outSize.v = thickness;
				outSize.h = adjacentPaneSize.h - fTopOffset - fBottomOffset;
				}
			else
				{
				outSize.v = adjacentPaneSize.v - fTopOffset - fBottomOffset;
				outSize.h = thickness;
				}
			return true;
			}
		}
	return ZPaneLocator::GetPaneSize(inPane, outSize);
	}

bool ZUIScrollableScrollBarConnector::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inPane == fScrollBar)
		{
		if (inAttribute == "ZUIRange::Value")
			{
			ZPoint_T<double> scrollValues = fScrollable->GetScrollValues();
			if (fHorizontal)
				(*(double*)outResult) = scrollValues.h;
			else
				(*(double*)outResult) = scrollValues.v;
			return true;
			}
		if (inAttribute == "ZUIScrollBar::ThumbProportion")
			{
			ZPoint_T<double> thumbProportions = fScrollable->GetScrollThumbProportions();
			if (fHorizontal)
				(*(double*)outResult) = thumbProportions.h;
			else
				(*(double*)outResult) = thumbProportions.v;
			return true;
			}
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

// =================================================================================================

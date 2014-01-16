/*  @(#) $Id: ZUIDisplayState.h,v 1.2 2003/03/16 00:59:34 agreen Exp $ */

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

#ifndef __ZUIDisplayState__
#define __ZUIDisplayState__ 1
#include "zconfig.h"

#include "ZPane.h" // Mainly for EZTriState

class ZUIDisplayState
	{
public:
	ZUIDisplayState(bool iPressed, EZTriState iHilite, bool iEnabled, bool iFocused, bool iActive)
	:	fPressed(iPressed), fHilite(iHilite), fEnabled(iEnabled), fFocused(iFocused), fActive(iActive)
		{}
	ZUIDisplayState(bool iPressed, EZTriState iHilite, bool iEnabled)
	:	fPressed(iPressed), fHilite(iHilite), fEnabled(iEnabled), fFocused(false), fActive(false)
		{}
	ZUIDisplayState()
	:	fPressed(false), fHilite(eZTriState_Normal), fEnabled(false), fFocused(false), fActive(false)
		{}

	bool GetPressed() const
		{ return fPressed; }
	EZTriState GetHilite() const
		{ return fHilite; }
	bool GetEnabled() const
		{ return fEnabled; }
	bool GetFocused() const
		{ return fFocused; }
	bool GetActive() const
		{ return fActive; }

	void SetPressed(bool pressed)
		{ fPressed = pressed; }
	void SetHilite(EZTriState hilite)
		{ fHilite = hilite; }
	void SetEnabled(bool enabled)
		{ fEnabled = enabled; }
	void SetFocused(bool focused)
		{ fFocused = focused; }
	void SetActive(bool active)
		{ fActive = active; }

protected:
	bool fPressed;
	EZTriState fHilite;
	bool fEnabled;
	bool fFocused;
	bool fActive;
	};

#endif // __ZUIDisplayState__

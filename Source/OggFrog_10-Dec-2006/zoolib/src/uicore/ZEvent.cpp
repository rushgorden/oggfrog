static const char rcsid[] = "@(#) $Id: ZEvent.cpp,v 1.3 2006/07/12 19:41:08 agreen Exp $";

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

#include "ZEvent.h"
#include "ZDebug.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZEvent_Key

ZEvent_Key::ZEvent_Key(const ZMessage& inMessage)
:	fMessage(inMessage)
	{}

ZKeyboard::CharCode ZEvent_Key::GetCharCode() const
	{
	return (ZKeyboard::CharCode)fMessage.GetInt32("charCode");
	}

ZKeyboard::KeyCode ZEvent_Key::GetKeyCode() const
	{
	return (ZKeyboard::CharCode)fMessage.GetInt32("keyCode");
	}

bool ZEvent_Key::GetShift() const
	{ return fMessage.GetBool("shift"); }

bool ZEvent_Key::GetCommand() const
	{ return fMessage.GetBool("command"); }

bool ZEvent_Key::GetOption() const
	{ return fMessage.GetBool("option"); }

bool ZEvent_Key::GetControl() const
	{ return fMessage.GetBool("control"); }

bool ZEvent_Key::GetCapsLock() const
	{ return fMessage.GetBool("capsLock"); }

/*bool ZEvent_Key::GetMeta() const
	{
	bool theValue;
	if (fMessage.GetBool("meta", 0, theValue))
		return theValue;
	return false;
	}*/

// =================================================================================================
#pragma mark -
#pragma mark * ZEvent_Mouse

ZEvent_Mouse::ZEvent_Mouse(const ZMessage& inMessage)
:	fMessage(inMessage)
	{}

long ZEvent_Mouse::GetClickCount() const
	{ return fMessage.GetInt32("clicks"); }

ZPoint ZEvent_Mouse::GetLocation() const
	{ return fMessage.GetPoint("where"); }

bool ZEvent_Mouse::GetShift() const
	{ return fMessage.GetBool("shift"); }

bool ZEvent_Mouse::GetCommand() const
	{ return fMessage.GetBool("command"); }

bool ZEvent_Mouse::GetOption() const
	{ return fMessage.GetBool("option"); }

bool ZEvent_Mouse::GetControl() const
	{ return fMessage.GetBool("control"); }

bool ZEvent_Mouse::GetCapsLock() const
	{ return fMessage.GetBool("capsLock"); }

// This is an odd one. On Macintosh it's GetOption(), but on
// Windows it's GetControl(). These two buttons are used for much
// the same kind of alternative functionality on the two OSes. -ec 00.11.24
bool ZEvent_Mouse::GetMeta() const
	{
	if (ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon))
		return this->GetOption();

	else if (ZCONFIG(OS, Win32))
		return this->GetControl();

	else if ZCONFIG(OS, Be)
		return this->GetOption();

	else
		return false;
	}
